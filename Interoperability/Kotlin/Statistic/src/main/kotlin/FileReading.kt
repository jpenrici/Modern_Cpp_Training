// Statistic/src/main/kotlin/FileReading.kt
package statistic

import kotlinx.cinterop.addressOf
import kotlinx.cinterop.convert
import kotlinx.cinterop.usePinned
import platform.posix.fclose
import platform.posix.ferror
import platform.posix.fopen
import platform.posix.fread

/** Raised when a file cannot be opened or read. */
class FileReadException(message: String) : RuntimeException(message)

private const val BUFFER_SIZE = 64 * 1024

/**
 * Reads a whole file as text using plain POSIX calls (no extra dependency).
 *
 * Bytes that are not valid UTF-8 are replaced by U+FFFD instead of failing:
 * the input is expected to contain garbage, and the parser skips it later.
 *
 * @throws FileReadException if the file cannot be opened or a read error occurs.
 */
fun readTextFile(path: String): String {
    val file = fopen(path, "rb") ?: throw FileReadException("cannot open '$path'")

    val chunks = mutableListOf<ByteArray>()
    var total = 0
    try {
        val buffer = ByteArray(BUFFER_SIZE)
        while (true) {
            val read = buffer.usePinned { pinned ->
                fread(pinned.addressOf(0), 1.convert(), BUFFER_SIZE.convert(), file).toInt()
            }
            if (read > 0) {
                chunks += buffer.copyOf(read)
                total += read
            }
            if (read < BUFFER_SIZE) { // EOF or error
                if (ferror(file) != 0) throw FileReadException("read error on '$path'")
                break
            }
        }
    } finally {
        fclose(file)
    }

    val bytes = ByteArray(total)
    var offset = 0
    for (chunk in chunks) {
        chunk.copyInto(bytes, offset)
        offset += chunk.size
    }
    return bytes.decodeToString()
}
