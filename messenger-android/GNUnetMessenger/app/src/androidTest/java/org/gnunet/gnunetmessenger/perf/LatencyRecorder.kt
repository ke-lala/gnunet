package org.gnunet.gnunetmessenger.perf

import java.util.concurrent.ConcurrentHashMap
import kotlin.math.max
import kotlin.math.min

/**
 * Records per-message send→receive latencies keyed by sequence id.
 *
 * Intended usage inside an AndroidJUnit4 instrumented test:
 *   val rec = LatencyRecorder()
 *   rec.markSend(seq)                // right before sendText
 *   // ...somewhere in the message callback:
 *   rec.markReceive(seq)             // first time seq is seen
 *   val stats = rec.summarize()
 */
class LatencyRecorder {

    private val sendTimes = ConcurrentHashMap<Long, Long>()
    private val latenciesNs = java.util.Collections.synchronizedList(mutableListOf<Long>())

    fun markSend(seq: Long) {
        sendTimes[seq] = System.nanoTime()
    }

    /** Idempotent: extra callbacks for the same seq (duplicates, echoes) are ignored. */
    fun markReceive(seq: Long): Boolean {
        val sent = sendTimes.remove(seq) ?: return false
        val dtNs = System.nanoTime() - sent
        latenciesNs.add(dtNs)
        return true
    }

    fun receivedCount(): Int = latenciesNs.size
    fun outstanding(): Int = sendTimes.size

    fun summarize(totalWallNs: Long): PerfSummary {
        val snapshot: LongArray = synchronized(latenciesNs) {
            latenciesNs.toLongArray()
        }
        if (snapshot.isEmpty()) {
            return PerfSummary.empty(totalWallNs)
        }
        snapshot.sort()

        val n = snapshot.size
        var sum = 0L
        var lo = Long.MAX_VALUE
        var hi = Long.MIN_VALUE
        for (v in snapshot) {
            sum += v
            lo = min(lo, v)
            hi = max(hi, v)
        }

        val mean = sum / n
        fun percentile(p: Double): Long {
            val idx = ((n - 1) * p).toInt().coerceIn(0, n - 1)
            return snapshot[idx]
        }

        val p50 = percentile(0.50)
        val p95 = percentile(0.95)
        val p99 = percentile(0.99)

        val throughputMsgPerSec =
            if (totalWallNs > 0) n.toDouble() * 1_000_000_000.0 / totalWallNs.toDouble()
            else 0.0

        return PerfSummary(
            received = n,
            outstanding = sendTimes.size,
            minNs = lo,
            meanNs = mean,
            p50Ns = p50,
            p95Ns = p95,
            p99Ns = p99,
            maxNs = hi,
            totalWallNs = totalWallNs,
            throughputMsgPerSec = throughputMsgPerSec,
        )
    }
}

data class PerfSummary(
    val received: Int,
    val outstanding: Int,
    val minNs: Long,
    val meanNs: Long,
    val p50Ns: Long,
    val p95Ns: Long,
    val p99Ns: Long,
    val maxNs: Long,
    val totalWallNs: Long,
    val throughputMsgPerSec: Double,
) {
    fun render(label: String): String {
        fun ms(ns: Long) = "%.2f ms".format(ns / 1_000_000.0)
        return buildString {
            appendLine("=== $label ===")
            appendLine("  received       : $received")
            appendLine("  outstanding    : $outstanding")
            appendLine("  wall clock     : ${ms(totalWallNs)}")
            appendLine("  throughput     : %.1f msg/s".format(throughputMsgPerSec))
            appendLine("  latency min    : ${ms(minNs)}")
            appendLine("  latency mean   : ${ms(meanNs)}")
            appendLine("  latency p50    : ${ms(p50Ns)}")
            appendLine("  latency p95    : ${ms(p95Ns)}")
            appendLine("  latency p99    : ${ms(p99Ns)}")
            appendLine("  latency max    : ${ms(maxNs)}")
        }
    }

    companion object {
        fun empty(totalWallNs: Long) = PerfSummary(
            received = 0, outstanding = 0,
            minNs = 0, meanNs = 0, p50Ns = 0, p95Ns = 0, p99Ns = 0, maxNs = 0,
            totalWallNs = totalWallNs, throughputMsgPerSec = 0.0,
        )
    }
}

private val PERF_SEQ_REGEX = Regex("""PERF-(\d+)""")

/** Returns the seq number embedded in [text] as `PERF-<digits>`, or null. */
fun extractPerfSeq(text: String?): Long? {
    if (text.isNullOrEmpty()) return null
    val m = PERF_SEQ_REGEX.find(text) ?: return null
    return m.groupValues[1].toLongOrNull()
}

/** Renders a sequence number as a fixed-width tag for embedding in message text. */
fun perfTag(seq: Long): String = "PERF-%010d".format(seq)
