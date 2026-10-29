// Statistic/src/main/kotlin/Stats.kt
package statistic

import kotlinx.cinterop.addressOf
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.toKString
import kotlinx.cinterop.usePinned
import statslib.StatsResult
import statslib.StatsStatus
import statslib.stats_compute
import statslib.stats_status_message

/**
 * Summary statistics of a flat vector of numbers.
 * [variance] is the POPULATION variance (divides by n), matching the C++ core.
 */
data class Stats(
    val mean: Double,
    val median: Double,
    val variance: Double,
    val stddev: Double,
)

/**
 * Thrown when libstatslib reports a failure.
 * [status] is the raw status code returned by the C API.
 */
class StatsException(
    val status: StatsStatus,
    message: String,
) : RuntimeException(message)

/**
 * Computes mean, median, variance and standard deviation by calling the native
 * `stats_compute` function.
 *
 * Memory model (no copies, no manual free):
 *  - the DoubleArray is *pinned*, so the GC cannot move it while C reads it;
 *  - the output struct lives in a [memScoped] arena that is released on exit;
 *  - the library allocates nothing that Kotlin would need to free.
 *
 * @throws StatsException on empty input, NaN/infinity, overflow or internal errors.
 */
fun DoubleArray.computeStats(): Stats {
    // Pinning requires an element to take the address of, so reject empty input here.
    if (isEmpty()) {
        throw StatsException(
            StatsStatus.STATS_ERR_EMPTY_INPUT,
            describe(StatsStatus.STATS_ERR_EMPTY_INPUT),
        )
    }

    val input = this
    return memScoped {
        val out = alloc<StatsResult>()
        val status = input.usePinned { pinned ->
            stats_compute(pinned.addressOf(0), input.size.toLong(), out.ptr)
        }
        if (status != StatsStatus.STATS_OK) {
            throw StatsException(status, describe(status))
        }
        // Copy the values out of native memory before the arena is released.
        Stats(out.mean, out.median, out.variance, out.stddev)
    }
}

/** Human-readable text of a status, taken from the native library itself. */
private fun describe(status: StatsStatus): String =
    "stats_compute failed: " + (stats_status_message(status)?.toKString() ?: "unknown status")
