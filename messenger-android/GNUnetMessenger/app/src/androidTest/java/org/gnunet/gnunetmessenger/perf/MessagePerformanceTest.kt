package org.gnunet.gnunetmessenger.perf

import android.util.Log
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.test.runTest
import kotlinx.coroutines.withContext
import kotlinx.coroutines.withTimeout
import kotlin.time.Duration.Companion.minutes
import org.gnunet.gnunetmessenger.model.ChatAccount
import org.gnunet.gnunetmessenger.model.ChatContext
import org.gnunet.gnunetmessenger.model.ChatHandle
import org.gnunet.gnunetmessenger.model.GnunetReturnValue
import org.gnunet.gnunetmessenger.model.MessageKind
import org.gnunet.gnunetmessenger.model.MessengerApp
import org.gnunet.gnunetmessenger.service.boundimpl.GnunetChatBoundService
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Assume.assumeTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Throughput / latency microbenchmark for sending TEXT messages through the
 * full client → IPC → libgnunetchat → daemon → callback pipeline.
 *
 * (this file): single-account group loopback. One connected account
 * publishes into a group it created, and receives each message back via the
 * chat callback. Exercises every layer except the cross-peer hop.
 */
@RunWith(AndroidJUnit4::class)
class MessagePerformanceTest {

    private val appContext = ApplicationProvider.getApplicationContext<android.content.Context>()
    private val gnunetChat = GnunetChatBoundService(appContext)

    private val recorder = LatencyRecorder()

    // Tuneable.
    private val warmupMessages = 10
    private val measuredMessages = 200

    /**
     * The GNUnet monolithic scheduler is started by the server's MainActivity,
     * not by the IPC service's onCreate. If the server app has never been
     * launched in this process-lifetime, GNUNET_CHAT_start aborts because the
     * scheduler is missing. Force-launch the server's MainActivity so the
     * daemon boots before we bind.
     */
    @Before
    fun bootServerDaemon() {
        val launch = appContext.packageManager.getLaunchIntentForPackage(SERVER_PACKAGE)
        if (launch != null) {
            launch.addFlags(android.content.Intent.FLAG_ACTIVITY_NEW_TASK)
            appContext.startActivity(launch)
            // Give the server process and its native daemon some time to come up.
            Thread.sleep(15_000)
        }
    }

    @After
    fun tearDown() = runTest(timeout = 1.minutes) {
        gnunetChat.unbind()
        delay(500)
    }

    @Test
    fun singleAccountGroupLoopback_measuresLatencyAndThroughput() = runTest(timeout = 5.minutes) {
        val allDone = CompletableDeferred<Unit>()
        val probeDone = CompletableDeferred<Unit>()
        val expectedTotal = warmupMessages + measuredMessages
        var seenCount = 0
        var firstRecvAtNs: Long = 0L
        var lastRecvAtNs: Long = 0L

        // Fail fast if the IPC server package is missing; otherwise the test
        // floods binder retries. See README / test setup for install steps.
        val pm = appContext.packageManager
        val serverInstalled = runCatching {
            pm.getPackageInfo(SERVER_PACKAGE, 0); true
        }.getOrDefault(false)
        assumeTrue(
            "GNUnet IPC server package '$SERVER_PACKAGE' is not installed on the device. " +
                "Install gnunet-android before running the perf test.",
            serverInstalled,
        )

        val handle: ChatHandle = gnunetChat.startChat(MessengerApp()) { _, msg ->
            // Diagnostic: log every non-TEXT kind so we see LOGIN/CREATED_ACCOUNT/etc.
            if (msg.kind != MessageKind.TEXT) {
                Log.d(TAG, "callback kind=${msg.kind} text='${msg.text?.take(40)}'")
                return@startChat
            }
            val seq = extractPerfSeq(msg.text) ?: return@startChat

            // Probe message — separate signal, doesn't count as measurement traffic.
            if (seq == PROBE_SEQ) {
                if (!probeDone.isCompleted) probeDone.complete(Unit)
                return@startChat
            }

            val now = System.nanoTime()
            val wasFirst = (firstRecvAtNs == 0L)
            if (wasFirst) firstRecvAtNs = now
            lastRecvAtNs = now

            if (seq >= warmupMessages) {
                recorder.markReceive(seq)
            }
            seenCount++
            if (seenCount >= expectedTotal && !allDone.isCompleted) {
                allDone.complete(Unit)
            }
        }
        try {
            // runTest uses a *virtual* scheduler: withTimeout/delay here would
            // advance virtual time and never wait for the real binder to come up.
            // Hop to a real dispatcher so we measure wall clock.
            withContext(Dispatchers.Default.limitedParallelism(1)) {
                withTimeout(20_000) { gnunetChat.awaitReady(handle) }
            }
        } catch (t: Throwable) {
            Log.e(TAG, "awaitReady failed — server bind or startChat crashed", t)
            throw AssertionError(
                "startChat did not produce a live handle within 20s. " +
                    "Check logcat for 'GnunetChatBoundService' (bind state) and " +
                    "'GnunetChatIpc' / 'NativeBridge' (server-side startup). " +
                    "Underlying cause: ${t.javaClass.simpleName}: ${t.message}",
                t,
            )
        }

        // Create + connect the lone account.
        val account = createAndConnectAccount(handle, "PerfAccount")

        // Create a group the account is already a member of.
        val group = gnunetChat.createGroup(handle, "perf-group-${System.currentTimeMillis()}")
        val groupCtx: ChatContext = gnunetChat.getGroupContext(group)
        // Give the group a moment to be fully set up on the server side.
        withContext(Dispatchers.Default.limitedParallelism(1)) { delay(2_000) }

        // Connectivity probe: send one message and wait for it to round-trip
        // before doing the perf run. If this fails, the daemon is wedged and
        // measuring 500 doomed sends would just be a 60s silent timeout.
        Log.i(TAG, "Sending loopback probe…")
        withContext(Dispatchers.IO) { gnunetChat.sendText(groupCtx, "${perfTag(PROBE_SEQ)} probe") }
        try {
            withContext(Dispatchers.Default.limitedParallelism(1)) {
                withTimeout(30_000) { probeDone.await() }
            }
            Log.i(TAG, "Probe round-tripped — daemon loopback is alive.")
        } catch (t: Throwable) {
            throw AssertionError(
                "Loopback probe did not return within 30s. The daemon is not " +
                    "echoing TEXT messages back through the callback. " +
                    "Try: `adb shell am force-stop $SERVER_PACKAGE && " +
                    "adb shell pm clear $SERVER_PACKAGE` and re-run.",
                t,
            )
        }

        Log.i(TAG, "Starting perf run: warmup=$warmupMessages measured=$measuredMessages")

        val wallStart = System.nanoTime()
        // Pipelined fire. LatencyRecorder handles out-of-order receive.
        for (seq in 0L until expectedTotal.toLong()) {
            if (seq >= warmupMessages) {
                recorder.markSend(seq)
            }
            val text = "${perfTag(seq)} hello"
            // sendText is runBlocking internally; this is a suspend-safe hot loop
            // because the loop body yields on each call.
            withContext(Dispatchers.IO) { gnunetChat.sendText(groupCtx, text) }
        }

        // Wait for everything to come back. Use real dispatcher (see note above).
        try {
            withContext(Dispatchers.Default.limitedParallelism(1)) {
                withTimeout(60_000) { allDone.await() }
            }
        } catch (t: Throwable) {
            Log.w(
                TAG,
                "Timed out waiting for all messages: received=${recorder.receivedCount()} " +
                    "outstanding=${recorder.outstanding()} seenCount=$seenCount",
            )
        }
        val wallEnd = lastRecvAtNs.takeIf { it != 0L } ?: System.nanoTime()

        val summary = recorder.summarize(totalWallNs = wallEnd - wallStart)
        val rendered = summary.render("MessagePerformanceTest / single-account group loopback")
        // Emit twice: once for logcat, once for the instrumentation stdout.
        Log.i(TAG, "\n$rendered")
        println(rendered)

        assertTrue(
            "Should have received >= 95% of measured messages (got ${summary.received}/$measuredMessages)",
            summary.received >= (measuredMessages * 0.95).toInt(),
        )
    }

    // ---- helpers ----

    private suspend fun createAndConnectAccount(
        handle: ChatHandle,
        name: String,
    ): ChatAccount {
        // Check if the account is already there from a previous run. Re-creating
        // the same name returns SYSERR and fails the assertion.
        val existing = runCatching { gnunetChat.listAccounts(handle) }
            .getOrDefault(emptyList())
        Log.i(TAG, "listAccounts (pre-create): ${existing.size} accounts " +
            existing.joinToString(",") { "'${it.name}'" })
        val pre = existing.firstOrNull { it.name.equals(name, ignoreCase = true) }
        if (pre != null) {
            Log.i(TAG, "Account '$name' already exists; using it.")
            gnunetChat.connect(handle, pre)
            withContext(Dispatchers.Default.limitedParallelism(1)) { delay(5_000) }
            return pre
        }

        val res = gnunetChat.createAccount(handle, name)
        Log.i(TAG, "createAccount('$name') -> $res")
        assertEquals("createAccount('$name')", GnunetReturnValue.OK, res)

        val account = withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(120_000) {
                var found: ChatAccount? = null
                var attempt = 0
                val acceptAnyAfterMs = 30_000L
                val started = System.currentTimeMillis()
                while (found == null) {
                    val accounts = runCatching { gnunetChat.listAccounts(handle) }
                        .getOrDefault(emptyList())
                    Log.i(TAG, "listAccounts attempt=$attempt size=${accounts.size} " +
                        accounts.joinToString(",") { "'${it.name}'" })
                    found = accounts.firstOrNull { it.name.equals(name, ignoreCase = true) }
                    if (found == null && accounts.isNotEmpty() &&
                        System.currentTimeMillis() - started > acceptAnyAfterMs
                    ) {
                        // Daemon didn't surface our exact name fast enough but it
                        // does have *some* account — use it. The perf test only
                        // needs a usable account, not specifically '$name'.
                        Log.w(TAG, "Falling back to first available account '${accounts[0].name}' " +
                            "after ${acceptAnyAfterMs/1000}s waiting for '$name'.")
                        found = accounts[0]
                    }
                    if (found == null) delay(500)
                    attempt++
                }
                found
            }
        }
        gnunetChat.connect(handle, account)
        withContext(Dispatchers.Default.limitedParallelism(1)) { delay(5_000) }
        return account
    }

    companion object {
        private const val TAG = "MessagePerfTest"
        private const val SERVER_PACKAGE = "org.gnu.gnunet"
        private const val PROBE_SEQ = Long.MAX_VALUE
    }
}
