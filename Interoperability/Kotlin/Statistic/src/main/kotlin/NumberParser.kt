// Statistic/src/main/kotlin/NumberParser.kt
package statistic

/** Valid numbers in reading order, plus how many tokens were rejected as garbage. */
class ParsedNumbers(
    val values: DoubleArray,
    val skippedTokens: Int,
)

// Same rules as the C++ reader (native/src/io/csv_reader.cpp):
// any run of these characters separates two tokens.
private val DELIMITERS = charArrayOf(' ', '\t', '\r', '\n', '\u000B', '\u000C', ',', ';')

// Strict decimal syntax. Kotlin's own toDoubleOrNull() is too permissive for
// garbage-filled input (it accepts "NaN", "Infinity", "1.0f", hex floats...),
// so a token must match this pattern BEFORE it is converted.
private val NUMBER = Regex("""[+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?""")

/**
 * Extracts every valid finite number from an irregular, garbage-filled text and
 * returns them as ONE flat vector (row/column structure is discarded).
 *
 *  - separators: comma, semicolon, space, tab, CR, LF (any mix);
 *  - rows may have different lengths or be empty;
 *  - a UTF-8 BOM is ignored;
 *  - invalid tokens and out-of-range numbers (e.g. 1e999) are skipped and counted.
 */
fun parseNumbers(text: String): ParsedNumbers {
    val values = ArrayList<Double>()
    var skipped = 0

    for (token in text.removePrefix("\uFEFF").split(*DELIMITERS)) {
        if (token.isEmpty()) continue // consecutive delimiters produce empty pieces

        val number = if (NUMBER.matches(token)) token.toDoubleOrNull() else null
        if (number != null && number.isFinite()) {
            values.add(number)
        } else {
            skipped++
        }
    }
    return ParsedNumbers(values.toDoubleArray(), skipped)
}
