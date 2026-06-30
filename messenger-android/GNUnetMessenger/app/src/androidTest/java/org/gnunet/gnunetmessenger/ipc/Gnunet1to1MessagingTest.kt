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
import org.gnunet.gnunetmessenger.model.ChatContact
import org.gnunet.gnunetmessenger.model.ChatContext
import org.gnunet.gnunetmessenger.model.ChatHandle
import org.gnunet.gnunetmessenger.model.ChatMessage
import org.gnunet.gnunetmessenger.model.MessageKind
import org.gnunet.gnunetmessenger.model.MessengerApp
import org.gnunet.gnunetmessenger.service.boundimpl.GnunetChatBoundService
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Assert.fail
import org.junit.Test
import kotlin.time.Duration.Companion.minutes
import org.junit.runner.RunWith

/**
 * End-to-end 1:1 messaging test. Pairs two accounts via lobby, then verifies
 * bidirectional text delivery: A -> B, then B -> A. Each direction asserts
 * the receiver sees a TEXT message whose body matches the sent text and whose
 * sender name matches the sending account.
 *
 * This is the regression test for the send-text bug that was fixed via the
 * separate `nativeContextPointer` field on ChatContext. If the IPC pointer
 * plumbing breaks again, sendText silently no-ops and the receiver wait
 * times out.
 */
@RunWith(AndroidJUnit4::class)
class Gnunet1to1MessagingTest {

    private val tag = "OneToOneMsg"
    private val appContext = ApplicationProvider.getApplicationContext<android.content.Context>()
    private val svcA = GnunetChatBoundService(appContext)
    private val svcB = GnunetChatBoundService(appContext)
    private val logA = mutableListOf<Pair<ChatContext, ChatMessage>>()
    private val logB = mutableListOf<Pair<ChatContext, ChatMessage>>()

    // libgnunetchat stores account names lower-cased; keep them lower-case
    // so receiver-side sender.name comparison matches.
    private val ts = System.currentTimeMillis()
    private val nameA = "msgtesta-$ts"
    private val nameB = "msgtestb-$ts"

    @After
    fun tearDown() = runTest {
        runCatching { svcA.unbind() }
        runCatching { svcB.unbind() }
        delay(1_000)
    }

    // ── Helpers ────────────────────────────────────────────────────────

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

    /**
     * Takes a snapshot of visible accounts via the fire-and-forget
     * [GnunetChatBoundService.iterateAccounts] call, with a small drain
     * delay so the binder callback can deliver results.
     */
    private suspend fun snapshotAccounts(
        svc: GnunetChatBoundService,
        handle: ChatHandle
    ): List<ChatAccount> {
        val acc = mutableListOf<ChatAccount>()
        svc.iterateAccounts(handle) { acc += it }
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
        Log.i(tag, "$label: account '${account.name}' visible; calling connect")

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

    /**
     * Wait until [receiverLog] contains a TEXT message whose body equals
     * [expectedText] AND whose sender name equals [expectedSenderName].
     * Returns the matching ChatMessage.
     */
    private suspend fun waitForIncomingText(
        receiverLabel: String,
        receiverLog: List<Pair<ChatContext, ChatMessage>>,
        expectedText: String,
        expectedSenderName: String,
        timeoutMs: Long = 60_000
    ): ChatMessage {
        Log.i(tag, "$receiverLabel: waiting for TEXT from '$expectedSenderName' body='$expectedText' (timeout=${timeoutMs}ms)")
        var match: ChatMessage? = null
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(timeoutMs) {
                while (match == null) {
                    match = receiverLog
                        .map { it.second }
                        .firstOrNull { msg ->
                            msg.kind == MessageKind.TEXT &&
                                msg.text == expectedText &&
                                msg.sender?.name?.equals(expectedSenderName, ignoreCase = true) == true
                        }
                    if (match == null) delay(200)
                }
            }
        }
        Log.i(tag, "$receiverLabel: received TEXT from '$expectedSenderName' body='$expectedText'")
        return match!!
    }

    // ── Test ───────────────────────────────────────────────────────────

    @Test
    fun bidirectionalTextMessageDeliversBetweenLobbyPairedAccounts() = runTest(timeout = 5.minutes) {
        // 5-minute wall-clock timeout so GNUnet DHT routing has time to
        // complete.  runTest's default 60 s is too short for first-time
        // P2P message delivery on a single device.
        Log.i(tag, "=== START: bidirectionalTextMessageDeliversBetweenLobbyPairedAccounts ===")

        // ── Step 1: Bring up two parallel chat handles ──────────────
        Log.i(tag, "Step 1: Starting two parallel chat handles")
        val handleA = svcA.startChat(MessengerApp()) { ctx, msg ->
            logA += ctx to msg
            Log.d(tag, "A onMessage: kind=${msg.kind} sender=${msg.sender?.name} text='${msg.text?.take(40)}'")
        }
        val handleB = svcB.startChat(MessengerApp()) { ctx, msg ->
            logB += ctx to msg
            Log.d(tag, "B onMessage: kind=${msg.kind} sender=${msg.sender?.name} text='${msg.text?.take(40)}'")
        }

        waitForHandle("A", handleA)
        waitForHandle("B", handleB)

        assertTrue("Handle A must be non-zero", handleA.pointer != 0L)
        assertTrue("Handle B must be non-zero", handleB.pointer != 0L)
        assertNotEquals(
            "Two startChat calls must produce distinct sessions",
            handleA.pointer,
            handleB.pointer
        )
        Log.i(tag, "Step 1 OK: handleA=${handleA.pointer} handleB=${handleB.pointer}")

        waitForRefresh("A", logA)
        waitForRefresh("B", logB)

        // ── Step 2: Create and connect accounts ─────────────────────
        Log.i(tag, "Step 2: Creating and connecting accounts")
        connectAccount("A", svcA, handleA, nameA, logA)
        connectAccount("B", svcB, handleB, nameB, logB)
        Log.i(tag, "Step 2 OK: both accounts connected")

        // ── Step 3: Pair via lobby ──────────────────────────────────
        Log.i(tag, "Step 3: Lobby pairing")
        Log.i(tag, "A: lobbyOpen")
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

        // ── Step 4: Wait for contact convergence ────────────────────
        Log.i(tag, "Step 4: Waiting for contact convergence")
        var contactsA: List<ChatContact> = emptyList()
        var contactsB: List<ChatContact> = emptyList()
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(60_000) {
                while (true) {
                    contactsA = svcA.listContacts(handleA)
                    contactsB = svcB.listContacts(handleB)
                    val aHasB = contactsA.any { it.name.equals(nameB, ignoreCase = true) }
                    val bHasA = contactsB.any { it.name.equals(nameA, ignoreCase = true) }
                    Log.d(tag, "poll: A.contacts=${contactsA.map { it.name }} B.contacts=${contactsB.map { it.name }}")
                    if (aHasB && bHasA) break
                    delay(500)
                }
            }
        }
        Log.i(tag, "Step 4 OK: A.contacts=${contactsA.map { it.name }} B.contacts=${contactsB.map { it.name }}")

        assertTrue(
            "A should have B as contact (got: ${contactsA.map { it.name }})",
            contactsA.any { it.name.equals(nameB, ignoreCase = true) }
        )
        assertTrue(
            "B should have A as contact (got: ${contactsB.map { it.name }})",
            contactsB.any { it.name.equals(nameA, ignoreCase = true) }
        )

        // Wait for CADET transport to establish between A and B. Contact
        // discovery (DHT/GNS) completes before the CADET channel is ready;
        // sending immediately risks the message being silently dropped.
        // Use Dispatchers.IO so the delay uses real wall-clock time — runTest
        // uses a virtual scheduler that makes plain delay() instant.
        Log.i(tag, "Step 4.5: waiting 30s for CADET transport to establish...")
        withContext(Dispatchers.IO) { delay(30_000) }
        Log.i(tag, "Step 4.5: CADET wait complete")

        // ── Step 5: Resolve 1:1 contexts ───────────────────────────
        Log.i(tag, "Step 5: Resolving 1:1 contexts")
        val contactBfromA = contactsA.first { it.name.equals(nameB, ignoreCase = true) }
        val contactAfromB = contactsB.first { it.name.equals(nameA, ignoreCase = true) }
        val ctxAtoB = svcA.getContactContext(contactBfromA)
        val ctxBtoA = svcB.getContactContext(contactAfromB)

        Log.i(tag, "Context A->B: nativePtr=${ctxAtoB.nativeContextPointer} userPtr=${ctxAtoB.userPointer}")
        Log.i(tag, "Context B->A: nativePtr=${ctxBtoA.nativeContextPointer} userPtr=${ctxBtoA.userPointer}")

        assertNotNull(
            "nativeContextPointer on A->B context must not be null",
            ctxAtoB.nativeContextPointer
        )
        assertNotNull(
            "nativeContextPointer on B->A context must not be null",
            ctxBtoA.nativeContextPointer
        )
        assertTrue(
            "nativeContextPointer on A->B must be non-empty (was '${ctxAtoB.nativeContextPointer}')",
            ctxAtoB.nativeContextPointer!!.isNotEmpty()
        )
        assertTrue(
            "nativeContextPointer on B->A must be non-empty (was '${ctxBtoA.nativeContextPointer}')",
            ctxBtoA.nativeContextPointer!!.isNotEmpty()
        )
        Log.i(tag, "Step 5 OK: nativeContextPointers validated")

        // ── Step 6: Send A -> B and verify receipt ──────────────────
        val bodyAtoB = "ping-from-A-$ts-${(1000..9999).random()}"
        Log.i(tag, "Step 6: A.sendText -> '$bodyAtoB'")
        svcA.sendText(ctxAtoB, bodyAtoB)

        // Give the native layer a moment to process, then verify the message
        // was stored locally in A's context before waiting on B to receive it.
        // Real-time delay — runTest's scheduler makes plain delay() virtual.
        withContext(Dispatchers.IO) { delay(3_000) }
        val localMsgsA = svcA.iterateContextMessages(ctxAtoB)
        Log.i(
            tag,
            "Step 6 local verify: A's context has ${localMsgsA.size} TEXT message(s) " +
                "after send: ${localMsgsA.map { "'${it.text}' kind=${it.kind}" }}"
        )
        if (localMsgsA.none { it.text == bodyAtoB }) {
            Log.e(tag, "Step 6 WARNING: sent message NOT found in A's local context — " +
                "nativeContextSendText may have failed or CADET was not ready")
        }

        val receivedOnB = waitForIncomingText(
            receiverLabel = "B",
            receiverLog = logB,
            expectedText = bodyAtoB,
            expectedSenderName = nameA,
            timeoutMs = 120_000
        )
        assertEquals("Body received on B must equal body sent by A", bodyAtoB, receivedOnB.text)
        assertEquals(
            "Sender on B's received message must be A",
            nameA.lowercase(),
            receivedOnB.sender?.name?.lowercase()
        )
        Log.i(tag, "Step 6 OK: A->B text delivered and verified")

        // ── Step 7: Send B -> A and verify receipt ──────────────────
        val bodyBtoA = "pong-from-B-$ts-${(1000..9999).random()}"
        Log.i(tag, "Step 7: B.sendText -> '$bodyBtoA'")
        svcB.sendText(ctxBtoA, bodyBtoA)

        withContext(Dispatchers.IO) { delay(3_000) }
        val localMsgsB = svcB.iterateContextMessages(ctxBtoA)
        Log.i(
            tag,
            "Step 7 local verify: B's context has ${localMsgsB.size} TEXT message(s) " +
                "after send: ${localMsgsB.map { "'${it.text}' kind=${it.kind}" }}"
        )
        if (localMsgsB.none { it.text == bodyBtoA }) {
            Log.e(tag, "Step 7 WARNING: sent message NOT found in B's local context — " +
                "nativeContextSendText may have failed or CADET was not ready")
        }

        val receivedOnA = waitForIncomingText(
            receiverLabel = "A",
            receiverLog = logA,
            expectedText = bodyBtoA,
            expectedSenderName = nameB,
            timeoutMs = 120_000
        )
        assertEquals("Body received on A must equal body sent by B", bodyBtoA, receivedOnA.text)
        assertEquals(
            "Sender on A's received message must be B",
            nameB.lowercase(),
            receivedOnA.sender?.name?.lowercase()
        )
        Log.i(tag, "Step 7 OK: B->A text delivered and verified")

        Log.i(tag, "=== PASS: bidirectional 1:1 text delivery verified ===")
    }
}