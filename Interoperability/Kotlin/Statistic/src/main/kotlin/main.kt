// Statistic/src/main/kotlin/main.kt
package statistic

import kotlin.system.exitProcess

/**
 * Reads a messy numeric file, flattens it to one vector and asks libstatslib.so
 * (C++23) for its statistics.
 *
 * Usage: statistic.kexe [path-to-file]     (default: data/sample.csv)
 */
fun main(args: Array<String>) {
    val path = args.firstOrNull() ?: "data/sample.csv"

    val parsed = try {
        parseNumbers(readTextFile(path))
    } catch (e: FileReadException) {
        println("error: ${e.message}")
        exitProcess(1)
    }

    println("file    : $path")
    println("numbers : ${parsed.values.size} valid, ${parsed.skippedTokens} skipped")

    try {
        val stats = parsed.values.computeStats()
        println("mean    : ${stats.mean}")
        println("median  : ${stats.median}")
        println("variance: ${stats.variance}  (population)")
        println("stddev  : ${stats.stddev}")
    } catch (e: StatsException) {
        println("error: ${e.message}")
        exitProcess(2)
    }
}
