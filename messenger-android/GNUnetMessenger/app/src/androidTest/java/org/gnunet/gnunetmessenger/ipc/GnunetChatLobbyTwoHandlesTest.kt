package org.gnunet.gnunetmessenger.ipc

import android.util.Log
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.test.runTest
import kotlinx.coroutines.withContext
import kotlinx.coroutines.withTimeout
import org.gnunet.gnunetmessenger.model.ChatAccount
import org.gnunet.gnunetmessenger.model.ChatContext
import org.gnunet.gnunetmessenger.model.ChatHandle
import org.gnunet.gnunetmessenger.model.ChatMessage
import org.gnunet.gnunetmessenger.model.GnunetReturnValue
import org.gnunet.gnunetmessenger.model.MessageKind
import org.gnunet.gnunetmessenger.model.MessengerApp
import org.gnunet.gnunetmessenger.service.boundimpl.GnunetChatBoundService
import org.junit.After
import org.junit.Assert.assertNotEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Validates the architectural hypothesis: lobby pairing succeeds when both
 * accounts are connected on parallel chat handles and neither side disconnects.
 */
@RunWith(AndroidJUnit4::class)
class GnunetChatLobbyTwoHandlesTest {

    private val tag = "TwoHandles"
    private val appContext = ApplicationProvider.getApplicationContext<android.content.Context>()
    private val svcA = GnunetChatBoundService(appContext)
    private val svcB = GnunetChatBoundService(appContext)
    private val logA = mutableListOf<Pair<ChatContext, ChatMessage>>()
    private val logB = mutableListOf<Pair<ChatContext, ChatMessage>>()

    // libgnunetchat stores account names lower-cased; keep them lower-case here
    // so equality checks against iterateAccounts results match.
    private val nameA = "twohandlea-${System.currentTimeMillis()}"
    private val nameB = "twohandleb-${System.currentTimeMillis()}"

    @After
    fun tearDown() = runTest {
        runCatching { svcA.unbind() }
        runCatching { svcB.unbind() }
        delay(1000)
    }

    private suspend fun waitForHandle(label: String, handle: ChatHandle, timeoutMs: Long = 30_000) {
        Log.i(tag, "$label: waitForHandle (timeout=${timeoutMs}ms)")
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(timeoutMs) {
                while (handle.pointer == 0L) delay(100)
            }
        }
        Log.i(tag, "$label: handle ready -> ${handle.pointer}")
    }

    private suspend fun waitForRefresh(
        label: String,
        log: List<Pair<ChatContext, ChatMessage>>,
        timeoutMs: Long = 30_000
    ) {
        Log.i(tag, "$label: waitForRefresh")
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(timeoutMs) {
                while (log.none { it.second.kind == MessageKind.REFRESH }) delay(100)
            }
        }
        Log.i(tag, "$label: REFRESH received")
    }

    private suspend fun snapshotAccounts(
        svc: GnunetChatBoundService,
        handle: ChatHandle
    ): List<ChatAccount> {
        val acc = mutableListOf<ChatAccount>()
        svc.iterateAccounts(handle) { acc += it }
        // iterateAccounts is one-shot; give the binder callback a moment to drain.
        delay(400)
        return acc.toList()
    }

    private suspend fun connectAccount(
        label: String,
        svc: GnunetChatBoundService,
        handle: ChatHandle,
        name: String,
        log: List<Pair<ChatContext, ChatMessage>>
    ): ChatAccount {
        Log.i(tag, "$label: createAccount('$name')")
        val rc = svc.createAccount(handle, name)
        Log.i(tag, "$label: createAccount returned $rc")

        Log.i(tag, "$label: polling iterateAccounts until '$name' appears")
        var found: ChatAccount? = null
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(45_000) {
                while (found == null) {
                    val snapshot = snapshotAccounts(svc, handle)
                    Log.d(tag, "$label: snapshot=${snapshot.map { it.name }}")
                    found = snapshot.firstOrNull { it.name.equals(name, ignoreCase = true) }
                    if (found == null) delay(500)
                }
            }
        }
        val account = found!!
        Log.i(tag, "$label: account '${account.name}' visible")

        Log.i(tag, "$label: connect('$name')")
        svc.connect(handle, account)

        Log.i(tag, "$label: waiting for LOGIN")
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(20_000) {
                while (log.none { it.second.kind == MessageKind.LOGIN }) delay(100)
            }
        }
        Log.i(tag, "$label: LOGIN received")
        return account
    }

    @Test
    fun pairingWithTwoSimultaneousHandlesProducesContactsOnBothSides() = runTest {
        Log.i(tag, "=== START: pairingWithTwoSimultaneousHandles ===")

        val handleA = svcA.startChat(MessengerApp()) { ctx, msg ->
            logA += ctx to msg
            Log.d(tag, "A onMessage: kind=${msg.kind} sender=${msg.sender?.name}")
        }
        val handleB = svcB.startChat(MessengerApp()) { ctx, msg ->
            logB += ctx to msg
            Log.d(tag, "B onMessage: kind=${msg.kind} sender=${msg.sender?.name}")
        }

        waitForHandle("A", handleA)
        waitForHandle("B", handleB)
        assertTrue(handleA.pointer != 0L)
        assertTrue(handleB.pointer != 0L)
        assertNotEquals(
            "Two startChat calls must produce distinct sessions",
            handleA.pointer,
            handleB.pointer
        )

        waitForRefresh("A", logA)
        waitForRefresh("B", logB)

        connectAccount("A", svcA, handleA, nameA, logA)
        connectAccount("B", svcB, handleB, nameB, logB)

        Log.i(tag, "Opening lobby on handleA")
        var lobbyUri = ""
        svcA.lobbyOpen(handleA) { uri ->
            Log.i(tag, "A: onLobbyUri received (${uri.length} chars)")
            lobbyUri = uri
        }
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(30_000) {
                while (lobbyUri.isEmpty()) delay(200)
            }
        }
        assertTrue("Lobby URI must be delivered", lobbyUri.isNotEmpty())
        Log.i(tag, "Lobby URI: ${lobbyUri.take(80)}...")

        Log.i(tag, "B: lobbyJoin")
        svcB.lobbyJoin(handleB, lobbyUri)

        Log.i(tag, "Waiting for both sides to converge on >=2 contacts each")
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(60_000) {
                while (true) {
                    val a = svcA.listContacts(handleA)
                    val b = svcB.listContacts(handleB)
                    Log.d(tag, "poll: A.contacts=${a.size} B.contacts=${b.size}")
                    if (a.size >= 2 && b.size >= 2) break
                    delay(500)
                }
            }
        }

        val contactsA = svcA.listContacts(handleA)
        val contactsB = svcB.listContacts(handleB)
        Log.i(tag, "A's contacts: ${contactsA.map { it.name }}")
        Log.i(tag, "B's contacts: ${contactsB.map { it.name }}")

        assertTrue(
            "A should have B as contact (got: ${contactsA.map { it.name }})",
            contactsA.any { it.name == nameB }
        )
        assertTrue(
            "B should have A as contact (got: ${contactsB.map { it.name }})",
            contactsB.any { it.name == nameA }
        )

        Log.i(tag, "A: createGroup('TestCrashGroup')")
        val group = svcA.createGroup(handleA, "TestCrashGroup")
        
        val contactB = contactsA.first { it.name == nameB }
        Log.i(tag, "A: inviteContactToGroup(group, contactB)")
        svcA.inviteContactToGroup(group, contactB)

        Log.i(tag, "Waiting to see if it crashes...")
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            delay(10000)
        }
    }
}
