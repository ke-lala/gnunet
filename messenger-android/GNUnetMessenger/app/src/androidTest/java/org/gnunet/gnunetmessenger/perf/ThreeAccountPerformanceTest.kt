package org.gnunet.gnunetmessenger.perf

import android.util.Log
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async
import kotlinx.coroutines.awaitAll
import kotlinx.coroutines.coroutineScope
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
 * Phase 2.5: three parallel sessions on the same daemon.
 *
 * Sole purpose: tell whether the daemon's serialization point (revealed by
 * Phase 2 — combined throughput stayed flat at ~100 msg/s with 2 sessions
 * instead of doubling) is a single bottleneck or a contention scaling effect.
 *
 *   - If 1/2/3-session combined throughput all sit near ~100 msg/s, that's
 *     a single serialization point (the GNUnet monolithic scheduler is the
 *     prime suspect — single-threaded, all sessions queue onto its loop).
 *   - If 3-session throughput drops below 2-session, contention scales
 *     super-linearly — likely lock contention rather than a clean queue.
 *
 * Design mirrors TwoAccountPerformanceTest: three independent
 * GnunetChatBoundService instances → three native sessions via g_sessions →
 * three accounts each with its own group, all firing concurrently with
 * disjoint seq ranges into a shared LatencyRecorder.
 */
@RunWith(AndroidJUnit4::class)
class ThreeAccountPerformanceTest {

    private val appContext = ApplicationProvider.getApplicationContext<android.content.Context>()
    private val clientAlpha = GnunetChatBoundService(appContext)
    private val clientBeta = GnunetChatBoundService(appContext)
    private val clientGamma = GnunetChatBoundService(appContext)

    private val recorder = LatencyRecorder()

    private val warmupPerClient = 10
    private val measuredPerClient = 80

    @Before
    fun bootServerDaemon() {
        val launch = appContext.packageManager.getLaunchIntentForPackage(SERVER_PACKAGE)
        if (launch != null) {
            launch.addFlags(android.content.Intent.FLAG_ACTIVITY_NEW_TASK)
            appContext.startActivity(launch)
            Thread.sleep(15_000)
        }
    }

    @After
    fun tearDown() = runTest(timeout = 1.minutes) {
        runCatching { clientAlpha.unbind() }
        runCatching { clientBeta.unbind() }
        runCatching { clientGamma.unbind() }
        delay(500)
    }

    @Test
    fun threeSessionParallelLoopback_measuresLatencyAndThroughput() = runTest(timeout = 5.minutes) {
        val pm = appContext.packageManager
        val serverInstalled = runCatching {
            pm.getPackageInfo(SERVER_PACKAGE, 0); true
        }.getOrDefault(false)
        assumeTrue(
            "GNUnet IPC server package '$SERVER_PACKAGE' is not installed.",
            serverInstalled,
        )

        val perClientTotal = warmupPerClient + measuredPerClient
        val sessions = listOf(
            SessionFixture(
                "PerfAccountAlpha", clientAlpha,
                seqStart = 0L * perClientTotal, probeSeq = Long.MAX_VALUE,
            ),
            SessionFixture(
                "PerfAccountBeta", clientBeta,
                seqStart = 1L * perClientTotal, probeSeq = Long.MAX_VALUE - 1,
            ),
            SessionFixture(
                "PerfAccountGamma", clientGamma,
                seqStart = 2L * perClientTotal, probeSeq = Long.MAX_VALUE - 2,
            ),
        )

        // Wire up callbacks + handles. Each callback only counts seqs in its own range.
        for (s in sessions) {
            s.handle = s.client.startChat(MessengerApp()) { _, msg ->
                if (msg.kind != MessageKind.TEXT) {
                    Log.d(TAG, "[${s.label}] kind=${msg.kind} text='${msg.text?.take(40)}'")
                    return@startChat
                }
                val seq = extractPerfSeq(msg.text) ?: return@startChat
                if (seq == s.probeSeq) {
                    if (!s.probe.isCompleted) s.probe.complete(Unit)
                    return@startChat
                }
                if (seq < s.seqStart || seq >= s.seqStart + perClientTotal) return@startChat

                val now = System.nanoTime()
                if (s.firstRecvNs == 0L) s.firstRecvNs = now
                s.lastRecvNs = now

                val warmupCutoff = s.seqStart + warmupPerClient
                if (seq >= warmupCutoff) recorder.markReceive(seq)
                s.seen++
                if (s.seen >= perClientTotal && !s.done.isCompleted) s.done.complete(Unit)
            }
        }

        // Wait for all three sessions to be live.
        try {
            withContext(Dispatchers.Default.limitedParallelism(sessions.size)) {
                withTimeout(30_000) {
                    coroutineScope {
                        sessions.map { s ->
                            async { s.client.awaitReady(s.handle) }
                        }.awaitAll()
                    }
                }
            }
        } catch (t: Throwable) {
            Log.e(TAG, "awaitReady failed for one of the three sessions", t)
            throw AssertionError(
                "Three-session startChat did not produce live handles within 30s. " +
                    "Underlying cause: ${t.javaClass.simpleName}: ${t.message}",
                t,
            )
        }

        // Create + connect one account per client.
        for (s in sessions) {
            s.account = createAndConnectAccount(s.client, s.handle, s.label)
        }

        // Each client gets its own loopback group.
        for (s in sessions) {
            val g = s.client.createGroup(
                s.handle, "perf-${s.label.lowercase()}-${System.currentTimeMillis()}",
            )
            s.ctx = s.client.getGroupContext(g)
        }
        withContext(Dispatchers.Default.limitedParallelism(1)) { delay(2_000) }

        // Connectivity probe per session.
        Log.i(TAG, "Sending loopback probes…")
        withContext(Dispatchers.IO) {
            for (s in sessions) {
                s.client.sendText(s.ctx!!, "${perfTag(s.probeSeq)} probe")
            }
        }
        try {
            withContext(Dispatchers.Default.limitedParallelism(sessions.size)) {
                withTimeout(45_000) {
                    coroutineScope {
                        sessions.map { s -> async { s.probe.await() } }.awaitAll()
                    }
                }
            }
            Log.i(TAG, "All ${sessions.size} probes round-tripped — daemon loopback is alive.")
        } catch (t: Throwable) {
            throw AssertionError(
                "Loopback probe did not return for one or more sessions within 45s. " +
                    sessions.joinToString(" ") { "${it.label}=${it.probe.isCompleted}" } +
                    ". Try `adb shell am force-stop $SERVER_PACKAGE && " +
                    "adb shell pm clear $SERVER_PACKAGE` and re-run.",
                t,
            )
        }

        Log.i(
            TAG,
            "Starting three-session perf run: warmup=$warmupPerClient measured=$measuredPerClient per session",
        )

        val wallStart = System.nanoTime()
        withContext(Dispatchers.Default.limitedParallelism(sessions.size)) {
            coroutineScope {
                sessions.map { s ->
                    async {
                        sendBatch(
                            client = s.client,
                            ctx = s.ctx!!,
                            seqStart = s.seqStart,
                            total = perClientTotal,
                            warmup = warmupPerClient,
                        )
                    }
                }.awaitAll()
            }
        }

        // Drain. 3× the work of Phase 2, so 3× the timeout.
        try {
            withContext(Dispatchers.Default.limitedParallelism(sessions.size)) {
                withTimeout(180_000) {
                    coroutineScope {
                        sessions.map { s -> async { s.done.await() } }.awaitAll()
                    }
                }
            }
        } catch (t: Throwable) {
            Log.w(
                TAG,
                "Timed out waiting for parallel loopback: " +
                    sessions.joinToString(" ") { "${it.label}=${it.seen}/$perClientTotal" } +
                    " received=${recorder.receivedCount()} outstanding=${recorder.outstanding()}",
            )
        }

        val wallEnd = sessions.maxOf { it.lastRecvNs }
            .takeIf { it != 0L } ?: System.nanoTime()
        val totalMeasured = measuredPerClient * sessions.size

        val summary = recorder.summarize(totalWallNs = wallEnd - wallStart)
        val rendered = summary.render(
            "ThreeAccountPerformanceTest / parallel three-session loopback " +
                "(3 × $perClientTotal msgs, $totalMeasured measured)",
        )
        Log.i(TAG, "\n$rendered")
        println(rendered)
        Log.i(
            TAG,
            "per-session counts: " +
                sessions.joinToString(" ") { "${it.label}=${it.seen}/$perClientTotal" },
        )

        assertTrue(
            "Should have received >= 95% of measured messages " +
                "(got ${summary.received}/$totalMeasured)",
            summary.received >= (totalMeasured * 0.95).toInt(),
        )
    }

    // ---- helpers ----

    private suspend fun sendBatch(
        client: GnunetChatBoundService,
        ctx: ChatContext,
        seqStart: Long,
        total: Int,
        warmup: Int,
    ) {
        for (i in 0 until total) {
            val seq = seqStart + i
            if (i >= warmup) recorder.markSend(seq)
            val text = "${perfTag(seq)} hello"
            withContext(Dispatchers.IO) { client.sendText(ctx, text) }
        }
    }

    private suspend fun createAndConnectAccount(
        client: GnunetChatBoundService,
        handle: ChatHandle,
        name: String,
    ): ChatAccount {
        val existing = runCatching { client.listAccounts(handle) }
            .getOrDefault(emptyList())
        Log.i(
            TAG, "[$name] listAccounts (pre-create): ${existing.size} accounts " +
                existing.joinToString(",") { "'${it.name}'" },
        )
        val pre = existing.firstOrNull { it.name.equals(name, ignoreCase = true) }
        if (pre != null) {
            Log.i(TAG, "[$name] account already exists; reusing.")
            client.connect(handle, pre)
            withContext(Dispatchers.Default.limitedParallelism(1)) { delay(5_000) }
            return pre
        }

        val res = client.createAccount(handle, name)
        Log.i(TAG, "[$name] createAccount -> $res")
        assertEquals("createAccount('$name')", GnunetReturnValue.OK, res)

        val account = withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(120_000) {
                var found: ChatAccount? = null
                var attempt = 0
                val acceptAnyAfterMs = 30_000L
                val started = System.currentTimeMillis()
                while (found == null) {
                    val accounts = runCatching { client.listAccounts(handle) }
                        .getOrDefault(emptyList())
                    Log.i(
                        TAG,
                        "[$name] listAccounts attempt=$attempt size=${accounts.size} " +
                            accounts.joinToString(",") { "'${it.name}'" },
                    )
                    found = accounts.firstOrNull { it.name.equals(name, ignoreCase = true) }
                    if (found == null && accounts.isNotEmpty() &&
                        System.currentTimeMillis() - started > acceptAnyAfterMs
                    ) {
                        Log.w(TAG, "[$name] Falling back to '${accounts[0].name}'.")
                        found = accounts[0]
                    }
                    if (found == null) delay(500)
                    attempt++
                }
                found
            }
        }
        client.connect(handle, account)
        withContext(Dispatchers.Default.limitedParallelism(1)) { delay(5_000) }
        return account
    }

    private class SessionFixture(
        val label: String,
        val client: GnunetChatBoundService,
        val seqStart: Long,
        val probeSeq: Long,
    ) {
        lateinit var handle: ChatHandle
        var account: ChatAccount? = null
        var ctx: ChatContext? = null
        val done = CompletableDeferred<Unit>()
        val probe = CompletableDeferred<Unit>()
        var seen = 0
        var firstRecvNs = 0L
        var lastRecvNs = 0L
    }

    companion object {
        private const val TAG = "ThreeAcctPerfTest"
        private const val SERVER_PACKAGE = "org.gnu.gnunet"
    }
}
