// Statistic/native/src/io/csv_reader.hpp

/**
 * @file csv_reader.hpp
 * @brief Tolerant reader that extracts a flat vector of numbers from messy text.
 *
 * The input is NOT required to be a well-formed CSV:
 *  - rows may have different numbers of columns (or be empty);
 *  - separators may be commas, semicolons, spaces, tabs or any mix of them;
 *  - tokens that are not a valid finite number are skipped (and counted);
 *  - LF, CRLF and a UTF-8 BOM are handled.
 *
 * Row/column structure is deliberately discarded: every valid number, in
 * reading order, is appended to a single std::vector<double>.
 *
 * The decimal separator is always '.', because ',' is a field separator.
 */
#pragma once

#include <cstddef>
#include <expected>
#include <filesystem>
#include <string_view>
#include <vector>

namespace statistic::io {

enum class CsvError {
    FileNotFound, ///< Path does not exist or is not a regular file.
    ReadFailure, ///< The file exists but could not be opened/read.
    NoNumericData ///< The text contained no valid number at all.
};

[[nodiscard]] constexpr auto to_string(CsvError error) noexcept -> std::string_view
{
    switch (error) {
    case CsvError::FileNotFound:
        return "FileNotFound";
    case CsvError::ReadFailure:
        return "ReadFailure";
    case CsvError::NoNumericData:
        return "NoNumericData";
    }
    return "Unknown";
}

struct CsvData {
    std::vector<double> values; ///< Valid numbers, in reading order.
    std::size_t skipped_tokens { 0 }; ///< Tokens rejected as garbage.
};

/// Parses an in-memory text. Never fails: garbage is skipped and counted.
[[nodiscard]] auto parse_numbers(std::string_view text) -> CsvData;

/// Reads a whole file and parses it. Fails if it cannot be read or holds no number.
[[nodiscard]] auto read_numbers(const std::filesystem::path& path)
    -> std::expected<CsvData, CsvError>;

} // namespace statistic::io
