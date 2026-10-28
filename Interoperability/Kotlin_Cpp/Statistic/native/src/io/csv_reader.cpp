// Statistic/native/src/io/csv_reader.hpp

/**
 * @file csv_reader.cpp
 * @brief Implementation of the tolerant number extractor (see csv_reader.hpp).
 */
#include "io/csv_reader.hpp"

#include <charconv>
#include <cmath>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <system_error>

namespace statistic::io {
namespace {

    // Any run of these characters separates two tokens.
    constexpr std::string_view kDelimiters = " \t\r\n\v\f,;";
    constexpr std::string_view kUtf8Bom = "\xEF\xBB\xBF";

    /// Strict conversion: the WHOLE token must be a finite decimal number.
    /// "1.0x", "0x1F", "nan", "inf", "-", "1e999" and "+-3" are all rejected.
    auto parse_token(std::string_view token) -> std::optional<double>
    {
        // std::from_chars does not accept a leading '+', so handle it by hand,
        // but refuse sign sequences such as "+-3" or "++3".
        if (token.starts_with('+')) {
            token.remove_prefix(1);
            if (token.empty() || token.front() == '+' || token.front() == '-')
                return std::nullopt;
        }

        double value {};
        const char* const first = token.data();
        const char* const last = first + token.size();
        const auto [ptr, ec] = std::from_chars(first, last, value);

        if (ec != std::errc {} || ptr != last) // invalid, out of range, or trailing junk
            return std::nullopt;
        if (!std::isfinite(value)) // from_chars happily parses "nan"/"inf"
            return std::nullopt;
        return value;
    }

} // namespace

auto parse_numbers(std::string_view text) -> CsvData
{
    CsvData out;

    if (text.starts_with(kUtf8Bom))
        text.remove_prefix(kUtf8Bom.size());

    std::size_t pos = 0;
    while ((pos = text.find_first_not_of(kDelimiters, pos)) != std::string_view::npos) {
        const std::size_t end = text.find_first_of(kDelimiters, pos);
        const std::size_t len = (end == std::string_view::npos) ? text.size() - pos : end - pos;

        if (auto value = parse_token(text.substr(pos, len)))
            out.values.push_back(*value);
        else
            ++out.skipped_tokens;

        pos += len;
    }
    return out;
}

auto read_numbers(const std::filesystem::path& path) -> std::expected<CsvData, CsvError>
{
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec) || ec)
        return std::unexpected(CsvError::FileNotFound);

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        return std::unexpected(CsvError::ReadFailure);

    const std::string text { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
    if (file.bad())
        return std::unexpected(CsvError::ReadFailure);

    auto data = parse_numbers(text);
    if (data.values.empty())
        return std::unexpected(CsvError::NoNumericData);
    return data;
}

} // namespace statistic::io
