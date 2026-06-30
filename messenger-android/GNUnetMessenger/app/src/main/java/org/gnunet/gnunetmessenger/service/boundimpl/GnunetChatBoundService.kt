package org.gnunet.gnunetmessenger.service.boundimpl

import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.os.DeadObjectException
import android.os.IBinder
import android.os.RemoteException
import android.util.Log
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withContext
import org.gnunet.gnunetmessenger.ipc.*
import org.gnunet.gnunetmessenger.model.*
import org.gnunet.gnunetmessenger.service.GnunetChat
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.atomic.AtomicReference

class GnunetChatBoundService(
    private val appContext: Context
) : GnunetChat {

    private var uuidCounter: Long = 0

    private val mainScope: CoroutineScope =
        CoroutineScope(SupervisorJob() + Dispatchers.Main.immediate)
    private val ioScope: CoroutineScope =
        CoroutineScope(SupervisorJob() + Dispatchers.IO)

    private val remoteRef = AtomicReference<IGnunetChat?>()
    private var deathRecipient: IBinder.DeathRecipient? = null

    @Volatile
    private var lastHandle: ChatHandle = ChatHandle(0L)

    @Volatile
    private lateinit var messageCallback: ((ChatContext, ChatMessage) -> Unit)

    private val pendingAfterHandle = mutableListOf<(IGnunetChat, Long) -> Unit>()

    private val handleReady = ConcurrentHashMap<ChatHandle, CompletableDeferred<Long>>()

    private val binderCallback = object : IChatCallback.Stub() {
        override fun onMessage(context: ChatContextDto, message: ChatMessageDto) {
            try {
                val ctxLocal = context.toLocal()
                val msgLocal = message.toLocal(ctxLocal)
                mainScope.launch { messageCallback.invoke(ctxLocal, msgLocal) }
            } catch (t: Throwable) {
                Log.e(TAG, "onMessage mapping failed", t)
            }
        }
    }

    private val conn = object : ServiceConnection {
        override fun onServiceConnected(name: ComponentName, service: IBinder) {
            val remote = IGnunetChat.Stub.asInterface(service)
            remoteRef.set(remote)

            val dr = IBinder.DeathRecipient {
                Log.w(TAG, "Remote binder died")
                remoteRef.set(null)
                lastHandle.pointer = 0L
                deathRecipient = null
            }
            deathRecipient = dr
            runCatching { service.linkToDeath(dr, 0) }
                .onFailure { Log.e(TAG, "linkToDeath failed", it) }
        }

        override fun onServiceDisconnected(name: ComponentName) {
            Log.w(TAG, "Remote disconnected")
            remoteRef.set(null)
            lastHandle.pointer = 0L
        }
    }

    init {
        bind()
    }

    private suspend fun getOrBindRemote(maxWaitMs: Long = 2_000): IGnunetChat {
        remoteRef.get()?.let { return it }
        bind()
        var waited = 0L
        while (waited < maxWaitMs) {
            remoteRef.get()?.let { return it }
            delay(100)
            waited += 100
        }
        throw IllegalStateException("Remote not connected (timeout)")
    }

    private suspend inline fun <T> withReadyRemote(
        handle: ChatHandle,
        retries: Int = 1,
        crossinline block: suspend (IGnunetChat, Long) -> T
    ): T {
        awaitReady(handle)
        var attempt = 0
        var lastError: Throwable? = null
        while (attempt <= retries) {
            val remote = try {
                getOrBindRemote()
            } catch (t: Throwable) {
                lastError = t
                null
            }

            if (remote != null) {
                try {
                    return block(remote, handle.pointer)
                } catch (dead: DeadObjectException) {
                    Log.w(TAG, "Binder died, rebinding… (attempt=$attempt)")
                    bind()
                    lastError = dead
                } catch (re: RemoteException) {
                    Log.w(TAG, "RemoteException, retry if possible (attempt=$attempt)", re)
                    bind()
                    lastError = re
                }
            } else {
                delay(150)
            }
            attempt++
        }
        throw RuntimeException("Remote call failed after retries", lastError)
    }

    private fun bind(): Boolean {
        val intent = Intent(ACTION_BIND_GNUNET_CHAT).setPackage(SERVER_PACKAGE)
        return appContext.bindService(intent, conn, Context.BIND_AUTO_CREATE)
    }

    /** True if the AIDL binder is currently connected. UI can use this to
     *  short-circuit before issuing a sync call that would otherwise crash
     *  the main thread when the server has died. */
    fun isRemoteAlive(): Boolean = remoteRef.get() != null

    /** Wraps a `runBlocking { withReadyRemote(...) }` body and converts the
     *  IllegalStateException / DeadObjectException-wrapped RuntimeException
     *  that fires when the server is gone into a logged warning + the
     *  caller-supplied default. Use only for methods whose callers can
     *  tolerate a stale answer (isGroup → false, getGroupFromContext → null,
     *  etc). For methods whose callers can't, let the throw propagate and
     *  the UI layer is expected to wrap them. */
    private inline fun <T> safeSync(default: T, label: String, block: () -> T): T = try {
        block()
    } catch (e: IllegalStateException) {
        Log.w(TAG, "$label: remote unavailable, returning default", e)
        default
    } catch (e: RuntimeException) {
        val cause = e.cause
        if (cause is IllegalStateException || cause is DeadObjectException ||
            cause is RemoteException
        ) {
            Log.w(TAG, "$label: remote call failed, returning default", e)
            default
        } else {
            throw e
        }
    }

    fun unbind() {
        val remote = remoteRef.get()
        val dr = deathRecipient
        if (remote != null && dr != null) {
            runCatching { remote.asBinder().unlinkToDeath(dr, 0) }
                .onFailure { Log.w(TAG, "unlinkToDeath failed", it) }
        }
        deathRecipient = null
        remoteRef.set(null)
        lastHandle.pointer = 0L
        runCatching { appContext.unbindService(conn) }
            .onFailure { Log.w(TAG, "unbindService failed", it) }
    }

    override suspend fun awaitReady(handle: ChatHandle) {
        if (handle.pointer != 0L) {
            if (remoteRef.get() != null) return
            Log.w(TAG, "awaitReady: remote lost, resetting stale handle=${handle.pointer}")
            handle.pointer = 0L
        }

        handleReady[handle]?.let { deferred ->
            try {
                val h = deferred.await()
                if (handle.pointer == 0L) {
                    handle.pointer = h
                }
                return
            } finally {
                handleReady.remove(handle)
            }
        }

        if (remoteRef.get() == null) {
            bind()
        }

        repeat(20) {
            if (remoteRef.get() != null) return@repeat
            delay(100)
        }

        val remote = remoteRef.get()
            ?: throw IllegalStateException("Remote not connected; startChat/bind() not completed")

        if (handle.pointer == 0L) {
            val real = remote.startChat(DEFAULT_APP_NAME, binderCallback)
            handle.pointer = real
            lastHandle.pointer = real
            handleReady[handle]?.complete(real)
            handleReady.remove(handle)
            drainPending(remote, real)
        }

        check(handle.pointer != 0L) { "Handle not ready (pointer==0)" }
    }

    override fun startChat(
        messengerApp: MessengerApp,
        callback: (ChatContext, ChatMessage) -> Unit
    ): ChatHandle {
        messageCallback = callback

        val ch = ChatHandle(0L)
        val deferred = CompletableDeferred<Long>()
        handleReady[ch] = deferred

        ioScope.launch {
            try {
                val remote = getOrBindRemote()
                val h = remote.startChat("messengerApp", binderCallback)
                lastHandle.pointer = h
                ch.pointer = h
                deferred.complete(h)
                drainPending(remote, h)
                Log.d(TAG, "startChat -> handle=$h")
            } catch (t: Throwable) {
                Log.e(TAG, "startChat failed", t)
                deferred.completeExceptionally(t)
            }
        }

        return ch
    }

    override suspend fun reset() = withContext(Dispatchers.IO) {
        val remote = getOrBindRemote()
        remote.reset()
        Log.i(TAG, "reset: successfully reset remote service")

        lastHandle.pointer = 0L
        handleReady.clear()
        synchronized(pendingAfterHandle) {
            pendingAfterHandle.clear()
        }
    }

    override fun iterateAccounts(handle: ChatHandle, callback: (ChatAccount) -> Unit) {
        val bridge = object : IAccountCallback.Stub() {
            override fun onAccount(accountDto: ChatAccountDto) {
                val acc = accountDto.toLocal()
                mainScope.launch { callback(acc) }
            }

            override fun onDone() {
                Log.d(TAG, "iterateAccounts: done")
            }

            override fun onError(code: Int, message: String?) {
                Log.e(TAG, "iterateAccounts: error $code $message")
            }
        }

        val remote = remoteRef.get()
        val h = lastHandle.takeIf { it.pointer != 0L } ?: handle

        when {
            remote != null && h.pointer != 0L -> {
                ioScope.launch {
                    try {
                        remote.iterateAccounts(h.pointer, bridge)
                    } catch (dead: DeadObjectException) {
                        Log.w(TAG, "iterateAccounts: binder died, queue & rebind")
                        synchronized(pendingAfterHandle) {
                            pendingAfterHandle += { r, real ->
                                runCatching { r.iterateAccounts(real, bridge) }
                                    .onFailure {
                                        Log.e(TAG, "iterateAccounts (deferred) failed", it)
                                    }
                            }
                        }
                        bind()
                    } catch (e: RemoteException) {
                        Log.e(TAG, "iterateAccounts remote failed", e)
                        synchronized(pendingAfterHandle) {
                            pendingAfterHandle += { r, real ->
                                runCatching { r.iterateAccounts(real, bridge) }
                                    .onFailure {
                                        Log.e(TAG, "iterateAccounts (deferred) failed", it)
                                    }
                            }
                        }
                        bind()
                    }
                }
            }

            remote != null && h.pointer == 0L -> {
                synchronized(pendingAfterHandle) {
                    pendingAfterHandle += { r, real ->
                        runCatching { r.iterateAccounts(real, bridge) }
                            .onFailure { Log.e(TAG, "iterateAccounts (deferred) failed", it) }
                    }
                }
            }

            else -> {
                synchronized(pendingAfterHandle) {
                    pendingAfterHandle += { r, real ->
                        runCatching { r.iterateAccounts(real, bridge) }
                            .onFailure { Log.e(TAG, "iterateAccounts (deferred) failed", it) }
                    }
                }
                bind()
            }
        }
    }

    override suspend fun listAccounts(handle: ChatHandle): List<ChatAccount> {
        return withReadyRemote(handle) { remote, h ->
            val result = mutableListOf<ChatAccount>()
            val done = CompletableDeferred<Unit>()

            val bridge = object : IAccountCallback.Stub() {
                override fun onAccount(accountDto: ChatAccountDto) {
                    result.add(accountDto.toLocal())
                }

                override fun onDone() {
                    if (!done.isCompleted) {
                        done.complete(Unit)
                    }
                }

                override fun onError(code: Int, message: String?) {
                    if (!done.isCompleted) {
                        done.completeExceptionally(
                            IllegalStateException("iterateAccounts failed: $code ${message ?: ""}".trim())
                        )
                    }
                }
            }

            remote.iterateAccounts(h, bridge)
            done.await()
            result.toList()
        }
    }

    private fun drainPending(remote: IGnunetChat, handle: Long) {
        val tasks = synchronized(pendingAfterHandle) {
            if (pendingAfterHandle.isEmpty()) return
            val copy = pendingAfterHandle.toList()
            pendingAfterHandle.clear()
            copy
        }
        ioScope.launch {
            tasks.forEach { task ->
                runCatching { task(remote, handle) }
                    .onFailure { Log.e(TAG, "deferred task failed", it) }
            }
        }
    }

    private fun Int.toGnunetReturn(): GnunetReturnValue =
        when (this) {
            0 -> GnunetReturnValue.OK
            else -> GnunetReturnValue.NO
        }

    override suspend fun createAccount(handle: ChatHandle, name: String): GnunetReturnValue {
        val code = withReadyRemote(handle) { remote, h ->
            withContext(Dispatchers.IO) { remote.createAccount(h, name) }
        }
        return when (code) {
            1 -> GnunetReturnValue.OK
            else -> GnunetReturnValue.NO
        }
    }

    override suspend fun connect(handle: ChatHandle, account: ChatAccount) {
        withReadyRemote(handle) { remote, h ->
            withContext(Dispatchers.IO) { remote.connect(h, account.toDto()) }
        }
    }

    override suspend fun disconnect(handle: ChatHandle) {
        withReadyRemote(handle) { remote, h ->
            withContext(Dispatchers.IO) { remote.disconnect(h) }
        }
    }

    override suspend fun stopChat(handle: ChatHandle) {
        withReadyRemote<Unit>(handle) { remote, h ->
            withContext(Dispatchers.IO) { remote.stopChat(h) }
        }
    }

    override suspend fun getProfileName(handle: ChatHandle): String {
        return withReadyRemote(handle) { remote, h ->
            withContext(Dispatchers.IO) { remote.getProfileName(h) ?: "" }
        }
    }

    override suspend fun setProfileName(handle: ChatHandle, name: String) {
        withReadyRemote(handle) { remote, h ->
            withContext(Dispatchers.IO) { remote.setProfileName(h, name) }
        }
    }

    override fun getProfileKey(handle: ChatHandle): String {
        return runBlocking {
            withReadyRemote(handle) { remote, h ->
                remote.getProfileKey(h)
            }
        }
    }

    override fun isContactBlocked(contact: ChatContact): Boolean {
        return runBlocking {
            withReadyRemote(lastHandle) { remote, _ ->
                remote.isContactBlocked(contact.toDto())
            }
        }
    }

    override fun setContactBlocked(contact: ChatContact, isBlocked: Boolean) {
        runBlocking {
            withReadyRemote(lastHandle) { remote, _ ->
                remote.setContactBlocked(contact.toDto(), isBlocked)
            }
        }
    }

    override fun setAttribute(handle: ChatHandle, key: String, value: String) {
        runBlocking {
            withReadyRemote(handle) { remote, h ->
                remote.setAttribute(h, key, value)
            }
        }
    }

    override fun getAttributes(handle: ChatHandle, callback: (String, String) -> Unit) {
        val bridge = object : IAttributeCallback.Stub() {
            override fun onAttribute(key: String, value: String) {
                mainScope.launch { callback(key, value) }
            }

            override fun onDone() {
                Log.d(TAG, "getAttributes: done")
            }

            override fun onError(code: Int, message: String?) {
                Log.e(TAG, "getAttributes: error $code $message")
            }
        }

        val remote = remoteRef.get()
        if (remote != null) {
            ioScope.launch {
                try {
                    remote.getAttributes(handle.pointer, bridge)
                } catch (e: RemoteException) {
                    Log.e(TAG, "getAttributes failed", e)
                }
            }
        } else {
            bind()
            synchronized(pendingAfterHandle) {
                pendingAfterHandle += { r, real ->
                    runCatching { r.getAttributes(real, bridge) }
                        .onFailure { Log.e(TAG, "getAttributes (deferred) failed", it) }
                }
            }
        }
    }

    override fun lobbyOpen(handle: ChatHandle, callback: (String) -> Unit) {
        val bridge = object : ILobbyCallback.Stub() {
            override fun onLobbyUri(uri: String) {
                mainScope.launch { callback(uri) }
            }

            override fun onError(code: Int, message: String?) {
                Log.e(TAG, "lobbyOpen: error $code $message")
            }
        }

        val remote = remoteRef.get()
        if (remote != null) {
            ioScope.launch {
                try {
                    remote.lobbyOpen(handle.pointer, bridge)
                } catch (e: RemoteException) {
                    Log.e(TAG, "lobbyOpen failed", e)
                }
            }
        } else {
            bind()
            synchronized(pendingAfterHandle) {
                pendingAfterHandle += { r, real ->
                    runCatching { r.lobbyOpen(real, bridge) }
                        .onFailure { Log.e(TAG, "lobbyOpen (deferred) failed", it) }
                }
            }
        }
    }

    override suspend fun lobbyJoin(handle: ChatHandle, uri: String) {
        withReadyRemote(handle) { remote, h ->
            withContext(Dispatchers.IO) { remote.lobbyJoin(h, uri) }
        }
    }

    override fun setGroupName(group: ChatGroup, name: String) {
        runBlocking {
            withReadyRemote(lastHandle) { remote, _ ->
                remote.setGroupName(group.toDto(), name)
            }
        }
    }

    override fun createGroup(handle: ChatHandle, topic: String): ChatGroup {
        return runBlocking {
            val groupDto = withReadyRemote(handle) { remote, h ->
                remote.createGroup(h, topic)
            }
            groupDto.toLocal()
        }
    }

    override fun parseUri(uri: String): ChatUri {
        return runBlocking {
            val uriDto = withReadyRemote(lastHandle) { remote, _ ->
                remote.parseUri(uri)
            }
            uriDto.toLocal()
        }
    }

    override fun destroyUri(uri: ChatUri) {
        runBlocking {
            withReadyRemote(lastHandle) { remote, _ ->
                remote.destroyUri(uri.toDto())
            }
        }
    }

    override fun inviteContactToGroup(group: ChatGroup, contact: ChatContact) {
        runBlocking {
            withReadyRemote(lastHandle) { remote, _ ->
                remote.inviteContactToGroup(group.toDto(), contact.toDto())
            }
        }
    }

    override fun getUserPointerForContext(context: ChatContext): String? {
        return runBlocking {
            withReadyRemote(lastHandle) { remote, _ ->
                remote.getUserPointerForContext(context.toDto())
            }
        }
    }

    override fun setUserPointerForContext(context: ChatContext, userPointer: String) {
        runBlocking {
            withReadyRemote(lastHandle) { remote, _ ->
                remote.setUserPointerForContext(context.toDto(), userPointer)
            }
        }
    }

    override fun getSenderFromMessage(message: ChatMessage): ChatContact {
        return runBlocking {
            val contactDto = withReadyRemote(lastHandle) { remote, _ ->
                remote.getSenderFromMessage(message.toDto())
            }
            contactDto.toLocal()
        }
    }

    override fun getGroupFromContext(context: ChatContext): ChatGroup? = safeSync(null, "getGroupFromContext") {
        runBlocking {
            val groupDto = withReadyRemote(lastHandle) { remote, _ ->
                remote.getGroupFromContext(context.toDto())
            }
            groupDto.toLocal()
        }
    }

    override fun getMessageForGroupContact(group: ChatGroup, contact: ChatContact): ChatMessage {
        return runBlocking {
            val messageDto = withReadyRemote(lastHandle) { remote, _ ->
                remote.getMessageForGroupContact(group.toDto(), contact.toDto())
            }
            messageDto.toLocal(ChatContext(null, null, false, false))
        }
    }

    override fun getMessageKind(message: ChatMessage): MessageKind {
        return runBlocking {
            val kind = withReadyRemote(lastHandle) { remote, _ ->
                remote.getMessageKind(message.toDto())
            }
            MessageKind.fromCode(kind)
        }
    }

    override fun isMessageRecent(message: ChatMessage): GnunetReturnValue {
        return runBlocking {
            val result = withReadyRemote(lastHandle) { remote, _ ->
                remote.isMessageRecent(message.toDto())
            }
            result.toGnunetReturn()
        }
    }

    override fun getMessageTimestamp(message: ChatMessage): Long {
        return runBlocking {
            withReadyRemote(lastHandle) { remote, _ ->
                remote.getMessageTimestamp(message.toDto())
            }
        }
    }

    override fun setMessageForGroupContact(
        group: ChatGroup,
        contact: ChatContact,
        message: ChatMessage
    ) {
        runBlocking {
            withReadyRemote(lastHandle) { remote, _ ->
                remote.setMessageForGroupContact(group.toDto(), contact.toDto(), message.toDto())
            }
        }
    }

    override suspend fun listContacts(handle: ChatHandle): List<ChatContact> {
        return withReadyRemote(handle) { remote, h ->
            val result = mutableListOf<ChatContact>()
            val done = CompletableDeferred<Unit>()

            val bridge = object : IContactCallback.Stub() {
                override fun onContact(contactDto: ChatContactDto) {
                    result.add(contactDto.toLocal())
                }

                override fun onDone() {
                    if (!done.isCompleted) {
                        done.complete(Unit)
                    }
                }

                override fun onError(code: Int, message: String?) {
                    if (!done.isCompleted) {
                        done.completeExceptionally(
                            IllegalStateException("iterateContacts failed: $code ${message ?: ""}".trim())
                        )
                    }
                }
            }

            remote.iterateContacts(h, bridge)
            done.await()
            result.toList()
        }
    }

    override suspend fun listGroups(handle: ChatHandle): List<ChatGroup> {
        return withReadyRemote(handle) { remote, h ->
            val result = mutableListOf<ChatGroup>()
            val done = CompletableDeferred<Unit>()

            val bridge = object : IGroupCallback.Stub() {
                override fun onGroup(groupDto: ChatGroupDto) {
                    result.add(groupDto.toLocal())
                }

                override fun onDone() {
                    if (!done.isCompleted) {
                        done.complete(Unit)
                    }
                }

                override fun onError(code: Int, message: String?) {
                    if (!done.isCompleted) {
                        done.completeExceptionally(
                            IllegalStateException("iterateGroups failed: $code ${message ?: ""}".trim())
                        )
                    }
                }
            }

            remote.iterateGroups(h, bridge)
            done.await()
            result.toList()
        }
    }

    override fun iterateContacts(handle: ChatHandle, callback: (ChatContact) -> Int) {
        val bridge = object : IContactCallback.Stub() {
            override fun onContact(contactDto: ChatContactDto) {
                val contact = contactDto.toLocal()
                mainScope.launch { callback(contact) }
            }

            override fun onDone() {
                Log.d(TAG, "iterateContacts: done")
            }

            override fun onError(code: Int, message: String?) {
                Log.e(TAG, "iterateContacts: error $code $message")
            }
        }

        val remote = remoteRef.get()
        if (remote != null) {
            ioScope.launch {
                try {
                    remote.iterateContacts(handle.pointer, bridge)
                } catch (e: RemoteException) {
                    Log.e(TAG, "iterateContacts failed", e)
                }
            }
        } else {
            bind()
            synchronized(pendingAfterHandle) {
                pendingAfterHandle += { r, real ->
                    runCatching { r.iterateContacts(real, bridge) }
                        .onFailure { Log.e(TAG, "iterateContacts (deferred) failed", it) }
                }
            }
        }
    }

    override fun iterateGroups(handle: ChatHandle, callback: (ChatGroup) -> Int) {
        val bridge = object : IGroupCallback.Stub() {
            override fun onGroup(groupDto: ChatGroupDto) {
                val group = groupDto.toLocal()
                mainScope.launch { callback(group) }
            }

            override fun onDone() {
                Log.d(TAG, "iterateGroups: done")
            }

            override fun onError(code: Int, message: String?) {
                Log.e(TAG, "iterateGroups: error $code $message")
            }
        }

        val remote = remoteRef.get()
        if (remote != null) {
            ioScope.launch {
                try {
                    remote.iterateGroups(handle.pointer, bridge)
                } catch (e: RemoteException) {
                    Log.e(TAG, "iterateGroups failed", e)
                }
            }
        } else {
            bind()
            synchronized(pendingAfterHandle) {
                pendingAfterHandle += { r, real ->
                    runCatching { r.iterateGroups(real, bridge) }
                        .onFailure { Log.e(TAG, "iterateGroups (deferred) failed", it) }
                }
            }
        }
    }

    override fun getContactContext(chatContact: ChatContact): ChatContext {
        return runBlocking {
            val contextDto = withReadyRemote(lastHandle) { remote, _ ->
                remote.getContactContext(chatContact.toDto())
            }
            contextDto.toLocal()
        }
    }

    override fun getGroupContext(chatGroup: ChatGroup): ChatContext {
        return runBlocking {
            val contextDto = withReadyRemote(lastHandle) { remote, _ ->
                remote.getGroupContext(chatGroup.toDto())
            }
            contextDto.toLocal()
        }
    }

    override fun getContactUserPointer(chatContact: ChatContact): String {
        return runBlocking {
            withReadyRemote(lastHandle) { remote, _ ->
                remote.getContactUserPointer(chatContact.toDto())
            }
        }
    }

    override fun setContactUserPointer(chatContact: ChatContact, userPointer: String) {
        runBlocking {
            withReadyRemote(lastHandle) { remote, _ ->
                remote.setContactUserPointer(chatContact.toDto(), userPointer)
            }
        }
    }

    override fun getGroupUserPointer(chatGroup: ChatGroup): String {
        return runBlocking {
            withReadyRemote(lastHandle) { remote, _ ->
                remote.getGroupUserPointer(chatGroup.toDto())
            }
        }
    }

    override fun setGroupUserPointer(chatGroup: ChatGroup, userPointer: String) {
        runBlocking {
            withReadyRemote(lastHandle) { remote, _ ->
                remote.setGroupUserPointer(chatGroup.toDto(), userPointer)
            }
        }
    }

    override fun sendText(chatContext: ChatContext, text: String) {
        val dto = chatContext.toDto()
        Log.d(
            TAG,
            "sendText[client]: nativeCtxPtr=${dto.nativeContextPointer} " +
                "userPtr=${dto.userPointer} textLen=${text.length} " +
                "lastHandle=${lastHandle.pointer}"
        )
        try {
            runBlocking {
                withReadyRemote(lastHandle) { remote, _ ->
                    Log.d(TAG, "sendText[client]: calling AIDL remote.sendText...")
                    remote.sendText(dto, text)
                    Log.d(TAG, "sendText[client]: AIDL remote.sendText returned OK")
                }
            }
        } catch (t: Throwable) {
            Log.e(TAG, "sendText[client]: AIDL call FAILED", t)
            throw t
        }
    }

    override fun getContactKey(chatContact: ChatContact): String {
        return runBlocking {
            withReadyRemote(lastHandle) { remote, _ ->
                remote.getContactKey(chatContact.toDto())
            }
        }
    }

    override fun getContextContact(context: ChatContext): ChatContact {
        return runBlocking {
            val contactDto = withReadyRemote(lastHandle) { remote, _ ->
                remote.getContextContact(context.toDto())
            }
            contactDto.toLocal()
        }
    }

    override fun deleteContact(chatContact: ChatContact) {
        runBlocking {
            withReadyRemote(lastHandle) { remote, _ ->
                remote.deleteContact(chatContact.toDto())
            }
        }
    }

    override fun isGroup(context: ChatContext): Boolean = safeSync(false, "isGroup") {
        runBlocking {
            withReadyRemote(lastHandle) { remote, _ ->
                remote.isGroup(context.toDto())
            }
        }
    }

    override fun isPlatform(context: ChatContext): Boolean = safeSync(false, "isPlatform") {
        runBlocking {
            withReadyRemote(lastHandle) { remote, _ ->
                remote.isPlatform(context.toDto())
            }
        }
    }

    override fun iterateGroupContacts(
        chatGroup: ChatGroup,
        callback: (ChatGroup, ChatContact) -> Int
    ) {
        val bridge = object : IGroupContactCallback.Stub() {
            override fun onGroupContact(groupDto: ChatGroupDto, contactDto: ChatContactDto) {
                val group = groupDto.toLocal()
                val contact = contactDto.toLocal()
                mainScope.launch { callback(group, contact) }
            }

            override fun onDone() {
                Log.d(TAG, "iterateGroupContacts: done")
            }

            override fun onError(code: Int, message: String?) {
                Log.e(TAG, "iterateGroupContacts: error $code $message")
            }
        }

        val remote = remoteRef.get()
        if (remote != null) {
            ioScope.launch {
                try {
                    remote.iterateGroupContacts(chatGroup.toDto(), bridge)
                } catch (e: RemoteException) {
                    Log.e(TAG, "iterateGroupContacts failed", e)
                }
            }
        } else {
            bind()
            synchronized(pendingAfterHandle) {
                pendingAfterHandle += { r, _ ->
                    runCatching { r.iterateGroupContacts(chatGroup.toDto(), bridge) }
                        .onFailure {
                            Log.e(TAG, "iterateGroupContacts (deferred) failed", it)
                        }
                }
            }
        }
    }

    override suspend fun listGroupContacts(group: ChatGroup): List<ChatContact> {
        return withReadyRemote(lastHandle) { remote, _ ->
            val result = mutableListOf<ChatContact>()
            val done = CompletableDeferred<Unit>()

            val bridge = object : IGroupContactCallback.Stub() {
                override fun onGroupContact(groupDto: ChatGroupDto, contactDto: ChatContactDto) {
                    result.add(contactDto.toLocal())
                }

                override fun onDone() {
                    if (!done.isCompleted) {
                        done.complete(Unit)
                    }
                }

                override fun onError(code: Int, message: String?) {
                    if (!done.isCompleted) {
                        done.completeExceptionally(
                            IllegalStateException("iterateGroupContacts failed: $code ${message ?: ""}".trim())
                        )
                    }
                }
            }

            remote.iterateGroupContacts(group.toDto(), bridge)
            done.await()
            result.toList()
        }
    }

    override fun randomUUID(): String {
        return "uuid_${System.currentTimeMillis()}_${uuidCounter++}"
    }

    override fun getContactAttributes(contact: ChatContact, callback: (String, String) -> Unit) {
        val bridge = object : IAttributeCallback.Stub() {
            override fun onAttribute(key: String, value: String) {
                mainScope.launch { callback(key, value) }
            }

            override fun onDone() {
                Log.d(TAG, "getContactAttributes: done")
            }

            override fun onError(code: Int, message: String?) {
                Log.e(TAG, "getContactAttributes: error $code $message")
            }
        }

        val remote = remoteRef.get()
        if (remote != null) {
            ioScope.launch {
                try {
                    remote.getContactAttributes(contact.toDto(), bridge)
                } catch (e: RemoteException) {
                    Log.e(TAG, "getContactAttributes failed", e)
                }
            }
        } else {
            bind()
            synchronized(pendingAfterHandle) {
                pendingAfterHandle += { r, _ ->
                    runCatching { r.getContactAttributes(contact.toDto(), bridge) }
                        .onFailure {
                            Log.e(TAG, "getContactAttributes (deferred) failed", it)
                        }
                }
            }
        }
    }

    override fun shareAttributes(handle: ChatHandle, contact: ChatContact, key: String) {
        runBlocking {
            withReadyRemote(handle) { remote, h ->
                remote.shareAttributes(h, contact.toDto(), key)
            }
        }
    }

    override fun unshareAttributes(handle: ChatHandle, contact: ChatContact, key: String) {
        runBlocking {
            withReadyRemote(handle) { remote, h ->
                remote.unshareAttributes(h, contact.toDto(), key)
            }
        }
    }

    override suspend fun iterateContextMessages(context: ChatContext): List<ChatMessage> {
        val messages = mutableListOf<ChatMessage>()
        val done = CompletableDeferred<Unit>()

        withReadyRemote(lastHandle) { remote, _ ->
            val cb = object : IMessageIterateCallback.Stub() {
                override fun onMessage(message: ChatMessageDto) {
                    val msg = message.toLocal(context)
                    messages.add(msg)
                }
                override fun onDone() {
                    done.complete(Unit)
                }
                override fun onError(code: Int, message: String?) {
                    done.completeExceptionally(
                        RuntimeException("iterateContextMessages failed: $code $message")
                    )
                }
            }
            remote.iterateContextMessages(context.toDto(), cb)
        }

        done.await()
        return messages
    }

    companion object {
        private const val TAG = "GnunetChatBoundService"
        private const val ACTION_BIND_GNUNET_CHAT =
            "org.gnunet.gnunetmessenger.ipc.BIND_GNUNET_CHAT"
        private const val SERVER_PACKAGE = "org.gnu.gnunet"
        private const val DEFAULT_APP_NAME = "Default"
    }
}