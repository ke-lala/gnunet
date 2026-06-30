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
 * Phase 2: two parallel sessions on the same daemon.
 *
 * Spins up two independent `GnunetChatBoundService` clients (so two native
 * GNUNET_CHAT_Handle instances via `g_sessions`), each with its own account
 * and its own group. Both fire half of the measured messages concurrently
 * into their own group and receive their own loopback. Latencies feed a
 * shared recorder via disjoint seq ranges.
 *
 * What this measures vs Phase 1:
 *  - Phase 1 = one client, one session, one account: pure pipeline cost.
 *  - Phase 2 = two clients, two sessions, two accounts under concurrent
 *    load: surfaces daemon-side serialization, lock contention, and IPC
 *    queue behavior.
 *
 * Note: this is *not* a cross-account A→B routing test. The existing
 * GnunetChatLobbyTest documents that lobby-join doesn't actually establish
 * a cross-account connection at the protocol level, so a true A→B test
 * would just time out at the JOIN step. Parallel loopback is the most
 * meaningful two-account experiment that actually completes end-to-end.
 */
@RunWith(AndroidJUnit4::class)
class TwoAccountPerformanceTest {

    private val appContext = ApplicationProvider.getApplicationContext<android.content.Context>()
    private val clientAlpha = GnunetChatBoundService(appContext)
    private val clientBeta = GnunetChatBoundService(appContext)

    private val recorder = LatencyRecorder()

    private val warmupPerClient = 10
    private val measuredPerClient = 100

    /**
     * Same boot-trick as Phase 1: the GNUnet scheduler is started by the
     * server's MainActivity, not by the IPC service's onCreate. Force-launch
     * it so the daemon is alive before either client binds.
     */
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
        delay(500)
    }

    @Test
    fun twoSessionParallelLoopback_measuresLatencyAndThroughput() = runTest(timeout = 5.minutes) {
        val pm = appContext.packageManager
        val serverInstalled = runCatching {
            pm.getPackageInfo(SERVER_PACKAGE, 0); true
        }.getOrDefault(false)
        assumeTrue(
            "GNUnet IPC server package '$SERVER_PACKAGE' is not installed on the device. " +
                "Install gnunet-android before running the perf test.",
            serverInstalled,
        )

        // --- per-session receive bookkeeping ---
        val perClientTotal = warmupPerClient + measuredPerClient
        val alphaSeqStart = 0L
        val betaSeqStart = perClientTotal.toLong()

        val alphaDone = CompletableDeferred<Unit>()
        val betaDone = CompletableDeferred<Unit>()
        val alphaProbe = CompletableDeferred<Unit>()
        val betaProbe = CompletableDeferred<Unit>()
        var alphaSeen = 0
        var betaSeen = 0
        var alphaFirstRecvNs = 0L
        var alphaLastRecvNs = 0L
        var betaFirstRecvNs = 0L
        var betaLastRecvNs = 0L

        val handleAlpha: ChatHandle = clientAlpha.startChat(MessengerApp()) { _, msg ->
            if (msg.kind != MessageKind.TEXT) {
                Log.d(TAG, "[alpha] kind=${msg.kind} text='${msg.text?.take(40)}'")
                return@startChat
            }
            val seq = extractPerfSeq(msg.text) ?: return@startChat
            if (seq == ALPHA_PROBE_SEQ) {
                if (!alphaProbe.isCompleted) alphaProbe.complete(Unit)
                return@startChat
            }
            // Only count messages this session emitted (its own loopback).
            if (seq < alphaSeqStart || seq >= alphaSeqStart + perClientTotal) return@startChat

            val now = System.nanoTime()
            if (alphaFirstRecvNs == 0L) alphaFirstRecvNs = now
            alphaLastRecvNs = now

            val warmupCutoff = alphaSeqStart + warmupPerClient
            if (seq >= warmupCutoff) recorder.markReceive(seq)
            alphaSeen++
            if (alphaSeen >= perClientTotal && !alphaDone.isCompleted) {
                alphaDone.complete(Unit)
            }
        }

        val handleBeta: ChatHandle = clientBeta.startChat(MessengerApp()) { _, msg ->
            if (msg.kind != MessageKind.TEXT) {
                Log.d(TAG, "[beta] kind=${msg.kind} text='${msg.text?.take(40)}'")
                return@startChat
            }
            val seq = extractPerfSeq(msg.text) ?: return@startChat
            if (seq == BETA_PROBE_SEQ) {
                if (!betaProbe.isCompleted) betaProbe.complete(Unit)
                return@startChat
            }
            if (seq < betaSeqStart || seq >= betaSeqStart + perClientTotal) return@startChat

            val now = System.nanoTime()
            if (betaFirstRecvNs == 0L) betaFirstRecvNs = now
            betaLastRecvNs = now

            val warmupCutoff = betaSeqStart + warmupPerClient
            if (seq >= warmupCutoff) recorder.markReceive(seq)
            betaSeen++
            if (betaSeen >= perClientTotal && !betaDone.isCompleted) {
                betaDone.complete(Unit)
            }
        }

        // Wait for both sessions to be live before doing anything else.
        try {
            withContext(Dispatchers.Default.limitedParallelism(2)) {
                withTimeout(20_000) {
                    coroutineScope {
                        val a = async { clientAlpha.awaitReady(handleAlpha) }
                        val b = async { clientBeta.awaitReady(handleBeta) }
                        awaitAll(a, b)
                    }
                }
            }
        } catch (t: Throwable) {
            Log.e(TAG, "awaitReady failed for one of the two sessions", t)
            throw AssertionError(
                "Two-session startChat did not produce live handles within 20s. " +
                    "Underlying cause: ${t.javaClass.simpleName}: ${t.message}",
                t,
            )
        }

        // Create + connect one account per client.
        val accountAlpha = createAndConnectAccount(clientAlpha, handleAlpha, "PerfAccountAlpha")
        val accountBeta = createAndConnectAccount(clientBeta, handleBeta, "PerfAccountBeta")

        // Each client gets its own loopback group.
        val groupAlpha = clientAlpha.createGroup(
            handleAlpha, "perf-alpha-${System.currentTimeMillis()}",
        )
        val groupBeta = clientBeta.createGroup(
            handleBeta, "perf-beta-${System.currentTimeMillis()}",
        )
        val ctxAlpha: ChatContext = clientAlpha.getGroupContext(groupAlpha)
        val ctxBeta: ChatContext = clientBeta.getGroupContext(groupBeta)
        withContext(Dispatchers.Default.limitedParallelism(1)) { delay(2_000) }

        // Connectivity probe per session before measuring.
        Log.i(TAG, "Sending loopback probes…")
        withContext(Dispatchers.IO) {
            clientAlpha.sendText(ctxAlpha, "${perfTag(ALPHA_PROBE_SEQ)} probe")
            clientBeta.sendText(ctxBeta, "${perfTag(BETA_PROBE_SEQ)} probe")
        }
        try {
            withContext(Dispatchers.Default.limitedParallelism(2)) {
                withTimeout(30_000) {
                    coroutineScope {
                        val a = async { alphaProbe.await() }
                        val b = async { betaProbe.await() }
                        awaitAll(a, b)
                    }
                }
            }
            Log.i(TAG, "Both probes round-tripped — daemon loopback is alive.")
        } catch (t: Throwable) {
            throw AssertionError(
                "Loopback probe did not return for one or both sessions within 30s. " +
                    "alphaProbe=${alphaProbe.isCompleted} betaProbe=${betaProbe.isCompleted}. " +
                    "Try `adb shell am force-stop $SERVER_PACKAGE && " +
                    "adb shell pm clear $SERVER_PACKAGE` and re-run.",
                t,
            )
        }

        Log.i(
            TAG,
            "Starting two-session perf run: warmup=$warmupPerClient measured=$measuredPerClient per session",
        )

        val wallStart = System.nanoTime()
        // Fire both sessions in parallel — same daemon, two binders, two groups.
        withContext(Dispatchers.Default.limitedParallelism(2)) {
            coroutineScope {
                val sendAlpha = async {
                    sendBatch(
                        client = clientAlpha,
                        ctx = ctxAlpha,
                        seqStart = alphaSeqStart,
                        total = perClientTotal,
                        warmup = warmupPerClient,
                    )
                }
                val sendBeta = async {
                    sendBatch(
                        client = clientBeta,
                        ctx = ctxBeta,
                        seqStart = betaSeqStart,
                        total = perClientTotal,
                        warmup = warmupPerClient,
                    )
                }
                awaitAll(sendAlpha, sendBeta)
            }
        }

        // Wait for both sides' loopback to drain.
        try {
            withContext(Dispatchers.Default.limitedParallelism(2)) {
                withTimeout(120_000) {
                    coroutineScope {
                        val a = async { alphaDone.await() }
                        val b = async { betaDone.await() }
                        awaitAll(a, b)
                    }
                }
            }
        } catch (t: Throwable) {
            Log.w(
                TAG,
                "Timed out waiting for parallel loopback: " +
                    "alphaSeen=$alphaSeen/$perClientTotal betaSeen=$betaSeen/$perClientTotal " +
                    "received=${recorder.receivedCount()} outstanding=${recorder.outstanding()}",
            )
        }

        val wallEnd = maxOf(alphaLastRecvNs, betaLastRecvNs)
            .takeIf { it != 0L } ?: System.nanoTime()
        val totalMeasured = measuredPerClient * 2

        val summary = recorder.summarize(totalWallNs = wallEnd - wallStart)
        val rendered = summary.render(
            "TwoAccountPerformanceTest / parallel two-session loopback " +
                "(2 × $perClientTotal msgs, ${measuredPerClient * 2} measured)",
        )
        Log.i(TAG, "\n$rendered")
        println(rendered)
        Log.i(
            TAG,
            "per-session counts: alpha=$alphaSeen/$perClientTotal beta=$betaSeen/$perClientTotal",
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

    companion object {
        private const val TAG = "TwoAcctPerfTest"
        private const val SERVER_PACKAGE = "org.gnu.gnunet"
        private const val ALPHA_PROBE_SEQ = Long.MAX_VALUE
        private const val BETA_PROBE_SEQ = Long.MAX_VALUE - 1
    }
}
