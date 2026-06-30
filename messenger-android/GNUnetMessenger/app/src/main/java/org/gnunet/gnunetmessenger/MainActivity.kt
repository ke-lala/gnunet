/*
   This file is part of GNUnet.
   Copyright (C) 2021--2025 GNUnet e.V.

   GNUnet is free software: you can redistribute it and/or modify it
   under the terms of the GNU Affero General Public License as published
   by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   GNUnet is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   SPDX-License-Identifier: AGPL3.0-or-later
 */
/*
 * @author t3sserakt
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/MainActivity.kt
 */

package org.gnunet.gnunetmessenger

import android.os.Bundle
import android.util.Log
import android.view.Menu
import android.view.MenuItem
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.Toolbar
import androidx.lifecycle.lifecycleScope
import androidx.navigation.NavController
import androidx.navigation.fragment.NavHostFragment
import androidx.navigation.ui.AppBarConfiguration
import androidx.navigation.ui.NavigationUI
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.isActive
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import kotlinx.coroutines.withContext
import kotlinx.coroutines.withTimeout
import java.util.concurrent.ConcurrentHashMap
import org.gnunet.gnunetmessenger.model.ChatAccount
import org.gnunet.gnunetmessenger.model.ChatContact
import org.gnunet.gnunetmessenger.model.ChatContext
import org.gnunet.gnunetmessenger.model.ChatHandle
import org.gnunet.gnunetmessenger.model.ChatMessage
import org.gnunet.gnunetmessenger.model.ChatMessageType
import org.gnunet.gnunetmessenger.model.ChatSummary
import org.gnunet.gnunetmessenger.model.MessageKind
import org.gnunet.gnunetmessenger.model.MessengerApp
import org.gnunet.gnunetmessenger.service.GnunetChat
import org.gnunet.gnunetmessenger.service.ServiceFactory
import org.gnunet.gnunetmessenger.viewmodel.ChatMenuViewModel
import org.gnunet.gnunetmessenger.viewmodel.ChatOverviewViewModel
import org.gnunet.gnunetmessenger.viewmodel.ChatViewModel
import org.gnunet.gnunetmessenger.viewmodel.ContactListViewModel

/**
 * One activated account's full state: its own bound-service instance, its
 * own chat handle on the daemon, and the user-facing account itself.
 *
 * **Multi-handle architecture (confirmed by upstream maintainer
 * Florian, 2026-05-14).** Each account the user activates gets its own
 * session, and all sessions stay live for the process lifetime — no
 * disconnect on switch. This mirrors how messenger-gtk runs multiple
 * client processes against one daemon. The previously observed group
 * crash correlates with a GNS-record serialization bug in GNUnet itself
 * (`gnsrecord_serialization.c:286` — "External protocol violation
 * detected") that Florian is fixing at the GNUnet layer.
 */
data class AccountSession(
    val account: ChatAccount,
    val handle: ChatHandle,
    val gnunetChat: GnunetChat
)

class MainActivity : AppCompatActivity() {

    private lateinit var gnunetChat: GnunetChat
    private lateinit var navController: NavController
    private lateinit var appBarConfiguration: AppBarConfiguration
    private lateinit var handle: ChatHandle

    /** Session registry keyed by lower-cased account name. */
    private val sessions = mutableMapOf<String, AccountSession>()

    private val chatReady = CompletableDeferred<ChatHandle>()
    private val initialRefreshReady = CompletableDeferred<Unit>()
    private val accountRefreshEvents = MutableSharedFlow<Unit>(extraBufferCapacity = 1)

    private val chatOverviewViewModel: ChatOverviewViewModel by viewModels()
    val contactListViewModel: ContactListViewModel by viewModels()

    private val chatMenuViewModels = mutableMapOf<String, ChatMenuViewModel>()
    private val chatViewModels = mutableMapOf<String, ChatViewModel>()
    private val chats = ConcurrentHashMap<String, ChatContext>()
    private val loadChatsMutex = Mutex()

    var currentAccount: ChatAccount? = null
        private set

    companion object {
        private const val TAG = "MainActivity"
        private const val CHAT_READY_POLL_MS = 50L
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        gnunetChat = ServiceFactory.create(this, useMock = false)

        val app = MessengerApp()
        handle = gnunetChat.startChat(app) { chatContext, chatMessage ->
            processChatMessage(chatContext, chatMessage)
        }

        lifecycleScope.launch {
            try {
                awaitHandlePointerReady()
                if (!chatReady.isCompleted) {
                    chatReady.complete(handle)
                }
                Log.d(TAG, "Chat handle ready: ${handle.pointer}")
            } catch (t: Throwable) {
                Log.e(TAG, "Chat initialization failed before first refresh", t)
                if (!chatReady.isCompleted) {
                    chatReady.completeExceptionally(t)
                }
                if (!initialRefreshReady.isCompleted) {
                    initialRefreshReady.completeExceptionally(t)
                }
            }
        }

        val navHostFragment =
            supportFragmentManager.findFragmentById(R.id.nav_host_fragment) as NavHostFragment
        navController = navHostFragment.navController

        val toolbar = findViewById<Toolbar>(R.id.nav_bar)
        setSupportActionBar(toolbar)

        appBarConfiguration = AppBarConfiguration(
            setOf(R.id.accountListFragment),
            null
        )

        NavigationUI.setupActionBarWithNavController(this, navController, appBarConfiguration)
    }

    override fun onStop() {
        super.onStop()
        
        lifecycleScope.launch {
            try {
                if (::handle.isInitialized && handle.pointer != 0L) {
                    Log.i(TAG, "Stopping chat session on onStop")
                    gnunetChat.stopChat(handle)
                }
            } catch (t: Throwable) {
                Log.e(TAG, "Failed to stop chat session in onStop", t)
            }
        }
    }

    private suspend fun awaitHandlePointerReady(): ChatHandle {
        while (lifecycleScope.coroutineContext.isActive && handle.pointer == 0L) {
            delay(CHAT_READY_POLL_MS)
        }
        check(handle.pointer != 0L) { "Chat handle was not initialized" }
        return handle
    }

    suspend fun awaitChatReady(): ChatHandle {
        return chatReady.await()
    }

    suspend fun awaitInitialDataReady(): ChatHandle {
        val readyHandle = chatReady.await()
        initialRefreshReady.await()
        return readyHandle
    }

    fun accountRefreshFlow(): SharedFlow<Unit> = accountRefreshEvents

    fun isChatReady(): Boolean {
        return chatReady.isCompleted && handle.pointer != 0L
    }

    fun hasInitialRefresh(): Boolean {
        return initialRefreshReady.isCompleted
    }

    private fun processChatMessage(chatContext: ChatContext, chatMessage: ChatMessage) {
        requireNotNull(chatMessage)

        // Shadow the singleton fields with the foreground session's instances
        // so the body of this handler always operates against the live account.
        val gnunetChat = getCurrentService()
        val handle = getCurrentHandle()

        Log.d(
            TAG,
            "processChatMessage: kind=${chatMessage.kind} " +
                "ctxPtr=${chatContext.userPointer} " +
                "sender=${chatMessage.sender?.name}/${chatMessage.sender?.key?.take(8)}"
        )

        when (chatMessage.kind) {
            MessageKind.WARNING -> {
            }

            MessageKind.REFRESH -> {
                Log.d(TAG, "Received REFRESH")
                if (!initialRefreshReady.isCompleted) {
                    initialRefreshReady.complete(Unit)
                    Log.d(TAG, "Initial refresh barrier released")
                }
                accountRefreshEvents.tryEmit(Unit)
            }

            MessageKind.LOGIN -> {
                loadChats()
            }

            MessageKind.LOGOUT -> {
                Log.d(TAG, "Received LOGOUT")
                // During account switch, clearChatState() is called explicitly
                // before disconnect(). Skip clearing here to avoid wiping data
                // that loadChats() (triggered by the subsequent LOGIN) has loaded.
                if (currentAccount == null) {
                    contactListViewModel.clearModel()
                    chatOverviewViewModel.clearModel()
                    chatViewModels.values.forEach { it.clearModel() }
                    chats.clear()
                }
            }

            MessageKind.CREATED_ACCOUNT,
            MessageKind.UPDATE_ACCOUNT -> {
            }

            MessageKind.UPDATE_CONTEXT -> {
            }

            MessageKind.JOIN,
            MessageKind.LEAVE -> {
                var uuid = gnunetChat.getUserPointerForContext(chatContext)
                val chatContact = chatMessage.sender
                    ?: gnunetChat.getSenderFromMessage(chatMessage)
                val contactName = if (chatContact.name.isNotBlank()) {
                    chatContact.name
                } else {
                    "A user"
                }
                val lastMessagePreview = if (chatMessage.kind == MessageKind.JOIN) {
                    "$contactName joined the chat"
                } else {
                    "$contactName left the chat"
                }
                val viewModel = getChatViewModel(chatContext)

                val displayTimestamp = if (chatMessage.timestamp == 0L) {
                    System.currentTimeMillis()
                } else {
                    chatMessage.timestamp
                }
                val displayMessage = chatMessage.copy(
                    text = lastMessagePreview,
                    type = ChatMessageType.SYSTEM,
                    timestamp = displayTimestamp
                )

                if (uuid == null) {
                    uuid = gnunetChat.randomUUID()
                    gnunetChat.setUserPointerForContext(chatContext, uuid)
                    chatContext.userPointer = uuid
                    chats[uuid] = chatContext
                    chatOverviewViewModel.addOrUpdateChat(
                        ChatSummary(chatContext, contactName, lastMessagePreview)
                    )
                } else {
                    chatContext.userPointer = uuid
                    if (chats[uuid] == null) {
                        chats[uuid] = chatContext
                    }
                    val displayContext = chats[uuid] ?: chatContext
                    val group = gnunetChat.getGroupFromContext(chatContext)
                    val displayName = if (group != null && group.name.isNotBlank()) {
                        group.name
                    } else {
                        contactName
                    }

                    chatOverviewViewModel.addOrUpdateChat(
                        ChatSummary(displayContext, displayName, lastMessagePreview)
                    )
                }
                viewModel?.addMessage(displayMessage)
            }

            MessageKind.CONTACT,
            MessageKind.SHARED_ATTRIBUTES -> {
                Log.d(TAG, "Received ${chatMessage.kind} — reloading chats so new contact appears")
                loadChats()
            }

            MessageKind.INVITATION -> {
                // Invitation is auto-accepted at the native layer.
                // Reload chats so the newly joined group appears in the overview.
                Log.d(TAG, "Received INVITATION — reloading chats to show new group")
                loadChats()
            }

            MessageKind.TEXT,
            MessageKind.FILE -> {
                val senderKey = chatMessage.sender?.key ?: ""
                val profileKey = gnunetChat.getProfileKey(handle)
                val ownEchoSkip = senderKey.isNotEmpty() && senderKey == profileKey
                Log.d(
                    TAG,
                    "TEXT/FILE foreground: senderKey='${senderKey.take(16)}...' " +
                        "(len=${senderKey.length}) profileKey='${profileKey.take(16)}...' " +
                        "(len=${profileKey.length}) match=$ownEchoSkip text='${chatMessage.text}'"
                )
                if (ownEchoSkip) {
                    // Already added this message to the view when we sent it,
                    // so skip the echo to avoid a duplicate.
                    return
                }

                val viewModel = getChatViewModel(chatContext)
                val uuid = gnunetChat.getUserPointerForContext(chatContext)
                val localChatContext = chats[uuid]
                chatMessage.type = ChatMessageType.OTHER

                if (localChatContext != null) {
                    viewModel?.addMessage(chatMessage)
                    chatOverviewViewModel.updateChatMessage(localChatContext, chatMessage)
                }
            }

            MessageKind.DELETION -> {
            }

            MessageKind.TAG -> {
            }

            MessageKind.ATTRIBUTES -> {
            }

            MessageKind.DISCOURSE -> {
            }

            MessageKind.DATA -> {
            }

            else -> {
                error("Unexpected message kind: ${chatMessage.kind}")
            }
        }
    }

    // Stable identifier for a chat context, independent of the native
    // pointer (which can drift between listGroups/listContacts refreshes
    // and is blank for newly-built contexts). This is what we key the
    // per-chat ViewModels by so that messages survive navigation.
    private fun stableChatKey(chatContext: ChatContext): String? {
        val gnunetChat = getCurrentService()
        runCatching { gnunetChat.getGroupFromContext(chatContext) }
            .getOrNull()
            ?.takeIf { it.name.isNotBlank() }
            ?.let { return "group:${it.name}" }

        runCatching { gnunetChat.getContextContact(chatContext) }
            .getOrNull()
            ?.key
            ?.takeIf { it.isNotBlank() }
            ?.let { return "contact:$it" }

        return chatContext.userPointer?.takeIf { it.isNotBlank() }
    }

    fun getChatViewModel(chatContext: ChatContext): ChatViewModel? {
        val base = stableChatKey(chatContext) ?: return null
        // Key the per-chat ViewModel by the *foreground* account too. OWN/OTHER
        // is viewer-relative: the same group message is "mine" for the account
        // that sent it and "theirs" for everyone else. A shared (account-blind)
        // ViewModel bakes one account's perspective into the stored message and
        // shows it wrong after a switch — the cause of "messages on the wrong
        // side for both accounts". One ViewModel per (account, chat) keeps each
        // account's perspective independent.
        val acct = currentAccount?.name?.lowercase() ?: "?"
        val key = "$acct|$base"
        val isNew = key !in chatViewModels
        val vm = chatViewModels.getOrPut(key) { ChatViewModel(chatContext) }
        Log.d(TAG, "getChatViewModel key=$key isNew=$isNew msgCount=${vm.messages.value?.size ?: 0}")
        return vm
    }

    fun getChatMenuViewModel(chatContext: ChatContext): ChatMenuViewModel? {
        val key = stableChatKey(chatContext) ?: return null
        return chatMenuViewModels.getOrPut(key) {
            ChatMenuViewModel(chatContext)
        }
    }

    fun loadChats() {
        lifecycleScope.launch {
            try {
                loadChatsSuspend()
            } catch (t: Throwable) {
                Log.e(TAG, "loadChats failed", t)
            }
        }
    }

    suspend fun loadChatsAndWait() {
        loadChatsSuspend()
    }

    private suspend fun loadChatsSuspend() = loadChatsMutex.withLock {
        // Capture the foreground session's instances; without this we'd
        // iterate against the bootstrap handle (no connected account → SYSERR).
        val gnunetChat = getCurrentService()
        val handle = getCurrentHandle()

        withContext(Dispatchers.IO) {
            val summaries = mutableListOf<ChatSummary>()
            val contacts = mutableListOf<ChatContact>()

            val contactList = gnunetChat.listContacts(handle)
            for (contact in contactList) {
                val chatContext = gnunetChat.getContactContext(contact)
                var uuid = gnunetChat.getUserPointerForContext(chatContext)
                if (uuid == null) {
                    uuid = gnunetChat.randomUUID()
                    gnunetChat.setUserPointerForContext(chatContext, uuid)
                }
                chatContext.userPointer = uuid
                contacts.add(contact)

                chats[uuid] = chatContext
                contact.blocked = gnunetChat.isContactBlocked(contact)
                val contactDisplayName = if (contact.name.isNotBlank()) {
                    contact.name
                } else {
                    val key = gnunetChat.getContactKey(contact)
                    if (key.isNotBlank()) "Contact (${key.take(8)}...)" else "Unknown contact"
                }
                summaries.add(
                    ChatSummary(
                        chatContext = chatContext,
                        displayName = contactDisplayName,
                        contact = contact
                    )
                )
            }

            val groupList = gnunetChat.listGroups(handle)
            val profileKey = gnunetChat.getProfileKey(handle)
            Log.d(TAG, "loadChats: profileKey=${profileKey.take(12)}, found ${groupList.size} groups: ${groupList.map { "${it.name}(ptr=${it.userPointer})" }}")
            for (group in groupList) {
                val chatContext = gnunetChat.getGroupContext(group)
                var uuid = gnunetChat.getUserPointerForContext(chatContext)
                if (uuid == null) {
                    uuid = gnunetChat.randomUUID()
                    gnunetChat.setUserPointerForContext(chatContext, uuid)
                }
                chatContext.userPointer = uuid

                chats[uuid] = chatContext

                val members = try {
                    gnunetChat.listGroupContacts(group)
                } catch (t: Throwable) {
                    Log.w(TAG, "listGroupContacts failed for ${group.name}", t)
                    emptyList()
                }

                // Log membership info for debugging.
                // NOTE: We no longer filter groups by membership here because
                // invited accounts may not yet appear in listGroupContacts()
                // until they send a JOIN message. The monolithic daemon exposes
                // all groups to all handles — a proper fix requires native-level
                // per-account context filtering.
                if (members.isNotEmpty()) {
                    Log.d(TAG, "Group '${group.name}' members: ${members.map { "${it.name}(${it.key.take(8)})" }}, profileKey=${profileKey.take(8)}")
                } else {
                    Log.d(TAG, "Group '${group.name}' has no members listed yet")
                }

                val namedMembers = members.filter { it.name.isNotBlank() }
                val memberPreview = if (namedMembers.isNotEmpty()) {
                    namedMembers.joinToString(", ") { it.name } + " joined the chat"
                } else if (members.isNotEmpty()) {
                    "${members.size} member(s) in the chat"
                } else {
                    null
                }

                val groupDisplayName = if (group.name.isNotBlank()) {
                    group.name
                } else {
                    "Lobby"
                }
                summaries.add(
                    ChatSummary(
                        chatContext = chatContext,
                        displayName = groupDisplayName,
                        lastMessagePreview = memberPreview,
                        group = group
                    )
                )
            }

            withContext(Dispatchers.Main) {
                contactListViewModel.setContacts(contacts)
                val existing = chatOverviewViewModel.chats.value.orEmpty()
                val merged = summaries.map { summary ->
                    val prev = existing.find {
                        it.chatContext.userPointer == summary.chatContext.userPointer
                    }
                    if (prev?.lastMessagePreview != null && summary.lastMessagePreview == null) {
                        summary.copy(lastMessagePreview = prev.lastMessagePreview)
                    } else {
                        summary
                    }
                }
                chatOverviewViewModel.setChats(merged)
            }
            Log.d(TAG, "loadChats: loaded ${contacts.size} contacts, ${groupList.size} groups")
        }
    }

    fun clearChatState() {
        chatOverviewViewModel.clearModel()
        contactListViewModel.clearModel()
        // Intentionally keep chatViewModels and chatMenuViewModels across
        // account switches. The per-chat message log is in-memory only, and
        // the daemon does not replay history on (re)connect. Wiping them
        // here makes the messages vanish the moment we switch accounts.
        // ViewModels are keyed per (account, chat) (see getChatViewModel),
        // so each account keeps its own correctly-sided copy of the
        // conversation; switching back restores that account's view intact.
        chats.clear()
        Log.d(TAG, "clearChatState: chat overview/contacts cleared, message history preserved")
    }

    override fun onCreateOptionsMenu(menu: Menu?): Boolean {
        menuInflater.inflate(R.menu.main_menu, menu)
        val current = currentAccount
        menu?.findItem(R.id.menu_current_account)?.title =
            "Account: ${current?.name ?: "No active account!"}"
        return true
    }

    override fun onPrepareOptionsMenu(menu: Menu?): Boolean {
        val current = currentAccount
        menu?.findItem(R.id.menu_current_account)?.title =
            "Account: ${current?.name ?: "No active account!"}"
        return super.onPrepareOptionsMenu(menu)
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        return when (item.itemId) {
            R.id.menu_current_account -> {
                if (currentAccount != null) {
                    val action = NavGraphDirections.actionGlobalAccountDetailsFragment()
                    navController.navigate(action)
                }
                true
            }

            R.id.menu_create_lobby -> {
                val action = NavGraphDirections.actionGlobalLobbyCreateFragment()
                navController.navigate(action)
                true
            }

            R.id.menu_join_lobby -> {
                val action = NavGraphDirections.actionGlobalLobbyJoinFragment()
                navController.navigate(action)
                true
            }

            R.id.menu_new_group -> {
                val action =
                    NavGraphDirections.actionGlobalCreateGroupOrPlatformFragment(isGroup = true)
                navController.navigate(action)
                true
            }

            R.id.menu_new_platform -> {
                val action =
                    NavGraphDirections.actionGlobalCreateGroupOrPlatformFragment(isGroup = false)
                navController.navigate(action)
                true
            }

            R.id.menu_contact_list -> {
                val action = NavGraphDirections.actionGlobalContactListFragment()
                navController.navigate(action)
                true
            }

            R.id.menu_settings -> {
                val action = NavGraphDirections.actionGlobalSettingsFragment()
                navController.navigate(action)
                true
            }

            R.id.menu_about -> true
            else -> super.onOptionsItemSelected(item)
        }
    }

    override fun onSupportNavigateUp(): Boolean {
        return NavigationUI.navigateUp(navController, appBarConfiguration) ||
                super.onSupportNavigateUp()
    }

    fun getGnunetChatInstance(): GnunetChat = getCurrentService()

    fun getChatHandle(): ChatHandle = getCurrentHandle()

    /** The session for the currently selected account, or null when none. */
    fun currentSession(): AccountSession? =
        currentAccount?.name?.lowercase()?.let { sessions[it] }

    /**
     * The bound-service instance the UI should use right now. Falls back
     * to the bootstrap singleton when no per-account session is registered.
     */
    fun getCurrentService(): GnunetChat = currentSession()?.gnunetChat ?: gnunetChat

    /**
     * The chat handle the UI should use right now. Falls back to the
     * bootstrap singleton handle when no per-account session is registered.
     */
    fun getCurrentHandle(): ChatHandle = currentSession()?.handle ?: handle

    fun setCurrentAccount(account: ChatAccount) {
        currentAccount = account
        invalidateOptionsMenu()
    }

    /**
     * Spawns a new bound-service + chat handle for [account], connects the
     * account on it, and registers the resulting [AccountSession] under
     * the lower-cased name. Idempotent — returns the existing session if
     * one is already registered. Both accounts stay live in libgnunetchat
     * simultaneously, mirroring how messenger-gtk runs two processes
     * against one daemon.
     */
    suspend fun spawnSessionFor(account: ChatAccount): AccountSession {
        val key = account.name.lowercase()
        sessions[key]?.let { return it }

        Log.d(TAG, "spawnSessionFor: starting session for '${account.name}'")
        val svc = ServiceFactory.create(applicationContext, useMock = false)
        val refreshSeen = CompletableDeferred<Unit>()
        val loginSeen = CompletableDeferred<Unit>()

        val newHandle = svc.startChat(MessengerApp()) { ctx, msg ->
            if (msg.kind == MessageKind.REFRESH && !refreshSeen.isCompleted) {
                refreshSeen.complete(Unit)
            }
            if (msg.kind == MessageKind.LOGIN && !loginSeen.isCompleted) {
                loginSeen.complete(Unit)
            }
            sessions[key]?.let { existing ->
                processChatMessageRouted(existing, ctx, msg)
            }
        }

        withTimeout(30_000) { while (newHandle.pointer == 0L) delay(50) }
        withTimeout(30_000) { refreshSeen.await() }

        Log.d(TAG, "spawnSessionFor: handle ready for '${account.name}', connecting")
        svc.connect(newHandle, account)
        withTimeout(20_000) { loginSeen.await() }

        val session = AccountSession(account, newHandle, svc)
        sessions[key] = session
        Log.d(TAG, "spawnSessionFor: '${account.name}' live (handle=${newHandle.pointer})")
        return session
    }

    /**
     * Routes a daemon message to [processChatMessage] only when [session]
     * is the foreground one. Background sessions receive events at the
     * libgnunetchat layer; switching to a background session triggers a
     * loadChats() that reads the up-to-date state from the daemon.
     */
    private fun processChatMessageRouted(
        session: AccountSession,
        chatContext: ChatContext,
        chatMessage: ChatMessage
    ) {
        val current = currentSession()
        val isForeground = current != null &&
            current.account.name.equals(session.account.name, ignoreCase = true)

        if (isForeground) {
            processChatMessage(chatContext, chatMessage)
            return
        }

        // Background session: route TEXT/FILE messages into the global chat
        // viewmodel using the session's own service so they're visible when
        // the user later switches to this session. Other event kinds will
        // be re-rendered via loadChats() on switch.
        when (chatMessage.kind) {
            MessageKind.TEXT, MessageKind.FILE -> {
                val senderKey = chatMessage.sender?.key ?: ""

                // Skip only THIS background session's own echo. ViewModels are
                // keyed per (account, chat), so each account holds its own copy
                // of the conversation. When account A (foreground) sends a group
                // message, B's background handler must record it as OTHER in B's
                // own ViewModel — that's how B sees A's message on the left after
                // switching. (The earlier "skip if sender is ANY of our sessions"
                // logic was for a single shared ViewModel and made B drop A's
                // messages entirely; with per-account ViewModels there's no cross-
                // account duplication to guard against.) The only echo to skip is
                // a message whose sender is this very session's account — B already
                // added that optimistically while B was foreground.
                val ownKey = runCatching {
                    session.gnunetChat.getProfileKey(session.handle)
                }.getOrDefault("")
                if (senderKey.isNotEmpty() && senderKey == ownKey) {
                    Log.d(
                        TAG,
                        "background-session ${session.account.name}: kind=${chatMessage.kind} " +
                            "(own echo, skip)"
                    )
                    return
                }

                val base = stableChatKeyFor(session, chatContext)
                if (base == null) {
                    Log.w(
                        TAG,
                        "background-session ${session.account.name}: kind=${chatMessage.kind} " +
                            "no stableChatKey — dropping"
                    )
                    return
                }
                // Same per-account keying scheme as getChatViewModel(), but using
                // this background session's account (not the foreground one).
                val key = "${session.account.name.lowercase()}|$base"
                val vm = chatViewModels.getOrPut(key) { ChatViewModel(chatContext) }
                chatMessage.type = ChatMessageType.OTHER
                vm.addMessage(chatMessage)
                Log.d(
                    TAG,
                    "background-session ${session.account.name}: kind=${chatMessage.kind} " +
                        "added to viewmodel '$key' (msgCount=${vm.messages.value?.size ?: 0})"
                )
            }
            else -> {
                Log.d(
                    TAG,
                    "background-session ${session.account.name}: kind=${chatMessage.kind} " +
                        "(deferred — will re-render on next switch)"
                )
            }
        }
    }

    /**
     * Same as [stableChatKey] but resolves group/contact info via the
     * given [session]'s service. Required when the message arrived on a
     * background session whose native context pointer is in that session's
     * address space — the foreground service can't dereference it.
     */
    private fun stableChatKeyFor(
        session: AccountSession,
        chatContext: ChatContext
    ): String? {
        val gnunetChat = session.gnunetChat
        runCatching { gnunetChat.getGroupFromContext(chatContext) }
            .getOrNull()
            ?.takeIf { it.name.isNotBlank() }
            ?.let { return "group:${it.name}" }

        runCatching { gnunetChat.getContextContact(chatContext) }
            .getOrNull()
            ?.key
            ?.takeIf { it.isNotBlank() }
            ?.let { return "contact:$it" }

        return chatContext.userPointer?.takeIf { it.isNotBlank() }
    }

    /**
     * Switches the foreground UI to [account]. Spawns a new session if
     * one doesn't exist. **Never disconnects existing sessions** — both
     * the lobby host and the joiner can stay live across the switch,
     * which is what makes lobby pairing succeed for both sides on a
     * single daemon.
     */
    suspend fun switchToSession(account: ChatAccount) {
        val key = account.name.lowercase()
        val previous = currentAccount
        val isSame = previous != null &&
            previous.name.equals(account.name, ignoreCase = true)
        if (isSame) return

        val session = sessions[key] ?: spawnSessionFor(account)

        runCatching {
            account.key = session.gnunetChat.getProfileKey(session.handle)
        }.onFailure { Log.w(TAG, "switchToSession: getProfileKey failed", it) }

        clearChatState()
        setCurrentAccount(account)
        runCatching { loadChatsAndWait() }
            .onFailure { Log.w(TAG, "switchToSession: loadChatsAndWait failed", it) }
    }
}