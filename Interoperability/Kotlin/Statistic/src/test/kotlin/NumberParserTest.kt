// Statistic/src/test/kotlin/NumberParserTest.kt
package statistic

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertTrue

class NumberParserTest {

    @Test
    fun irregularRowsAreFlattened() {
        // 3, 1, 0 (empty), 2 and 1 columns -> one flat vector in reading order.
        val r = parseNumbers("1.0 5.5 8.4\n2.3\n\n4.5 56.6\n0.0\n")
        assertContentEquals(doubleArrayOf(1.0, 5.5, 8.4, 2.3, 4.5, 56.6, 0.0), r.values)
        assertEquals(0, r.skippedTokens)
    }

    @Test
    fun mixedSeparators() {
        val r = parseNumbers("1,2;3\t4   5 ,; 6")
        assertContentEquals(doubleArrayOf(1.0, 2.0, 3.0, 4.0, 5.0, 6.0), r.values)
    }

    @Test
    fun garbageIsSkippedAndCounted() {
        // Eight rejected tokens; only "7" survives.
        val r = parseNumbers("abc 1.0x 0x1F nan inf - 1e999 +-3 7")
        assertContentEquals(doubleArrayOf(7.0), r.values)
        assertEquals(8, r.skippedTokens)
    }

    @Test
    fun numberFormats() {
        val r = parseNumbers("+1.5 -2 .5 5. 1e3 -2.5E-1")
        assertContentEquals(doubleArrayOf(1.5, -2.0, 0.5, 5.0, 1000.0, -0.25), r.values)
        assertEquals(0, r.skippedTokens)
    }

    @Test
    fun crlfBomAndNoTrailingNewline() {
        val r = parseNumbers("\uFEFF1.5\r\n2.5\r\n3.5")
        assertContentEquals(doubleArrayOf(1.5, 2.5, 3.5), r.values)
        assertEquals(0, r.skippedTokens)
    }

    @Test
    fun emptyAndGarbageOnlyText() {
        assertTrue(parseNumbers("").values.isEmpty())
        val g = parseNumbers("hello --- ### nan")
        assertTrue(g.values.isEmpty())
        assertEquals(4, g.skippedTokens)
    }
}
