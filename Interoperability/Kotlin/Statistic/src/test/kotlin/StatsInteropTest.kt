// Statistic/src/test/kotlin/StatsInteropTest.kt
package statistic

import kotlinx.cinterop.ExperimentalForeignApi
import platform.posix.fclose
import platform.posix.fopen
import platform.posix.fputs
import platform.posix.getpid
import platform.posix.remove
import statslib.STATS_ERR_EMPTY_INPUT
import statslib.STATS_ERR_NON_FINITE
import statslib.STATS_ERR_OVERFLOW
import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith

/** Exercises the real Kotlin -> libstatslib.so -> C++23 round trip. */
class StatsInteropTest {

    private val eps = 1e-9

    @Test
    fun knownDataSet() {
        // mean 5, median 4.5, population variance 4, stddev 2
        val s = doubleArrayOf(2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0).computeStats()
        assertEquals(5.0, s.mean, eps)
        assertEquals(4.5, s.median, eps)
        assertEquals(4.0, s.variance, eps)
        assertEquals(2.0, s.stddev, eps)
    }

    @Test
    fun singleElement() {
        val s = doubleArrayOf(42.0).computeStats()
        assertEquals(42.0, s.mean, eps)
        assertEquals(42.0, s.median, eps)
        assertEquals(0.0, s.variance, eps)
    }

    @Test
    fun inputArrayIsNotModified() {
        val data = doubleArrayOf(3.0, 1.0, 2.0)
        data.computeStats() // the C++ median works on a private copy
        assertContentEquals(doubleArrayOf(3.0, 1.0, 2.0), data)
    }

    @Test
    fun emptyInputIsRejected() {
        val e = assertFailsWith<StatsException> { DoubleArray(0).computeStats() }
        assertEquals(STATS_ERR_EMPTY_INPUT, e.status)
    }

    @Test
    fun nonFiniteInputIsReportedByNativeCode() {
        val e = assertFailsWith<StatsException> {
            doubleArrayOf(1.0, Double.NaN).computeStats()
        }
        assertEquals(STATS_ERR_NON_FINITE, e.status)
    }

    @Test
    fun overflowIsReportedByNativeCode() {
        val e = assertFailsWith<StatsException> {
            doubleArrayOf(Double.MAX_VALUE, Double.MAX_VALUE).computeStats()
        }
        assertEquals(STATS_ERR_OVERFLOW, e.status)
    }

    @Test
    fun largeArrayIsPassedWithoutCopy() {
        // 0, 1, ..., n-1: mean = (n-1)/2, population variance = (n^2 - 1) / 12
        val n = 1_000_000
        val s = DoubleArray(n) { it.toDouble() }.computeStats()
        assertEquals(499_999.5, s.mean, eps)
        assertEquals(499_999.5, s.median, eps)
        assertEquals(83_333_333_333.25, s.variance, 1.0) // accumulation error is far below 1.0
    }

    @Test
    fun missingFileFailsCleanly() {
        assertFailsWith<FileReadException> { readTextFile("/nonexistent/dir/file.csv") }
    }

    @OptIn(ExperimentalForeignApi::class)
    @Test
    fun messyFileEndToEnd() {
        // The 10 numbers of data/sample.csv plus 13 garbage tokens, irregular rows, CRLF.
        val text = "# sample data with garbage\n1.0 5.5 8.4\n2.3\n\nabc,4.5;56.6 1000.2\n,,\n" +
                "0.0\r\n1.2 1.0 1.0x NaN inf -\n0x1F @@ 1e999\n"
        val path = "/tmp/statistic_test_${getpid()}.csv"

        val file = fopen(path, "wb") ?: error("cannot create $path")
        try {
            fputs(text, file)
        } finally {
            fclose(file)
        }

        try {
            val parsed = parseNumbers(readTextFile(path))
            assertEquals(10, parsed.values.size)
            assertEquals(13, parsed.skippedTokens)

            // Reference values computed independently with Python's statistics module.
            val s = parsed.values.computeStats()
            assertEquals(108.07, s.mean, 1e-9)
            assertEquals(3.4, s.median, 1e-9)
            assertEquals(88_694.2141, s.variance, 1e-4)
            assertEquals(297.81574, s.stddev, 1e-4)
        } finally {
            remove(path)
        }
    }
}
