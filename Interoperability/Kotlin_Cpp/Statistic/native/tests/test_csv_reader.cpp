// Statistic/native/tests/test_csv_reader.cpp

/**
 * @file test_csv_reader.cpp
 * @brief Tests of the tolerant number extractor, including a messy file on disk.
 */
#include "minitest.hpp"

#include "core/stats.hpp"
#include "io/csv_reader.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace {

constexpr double kEps = 1e-9;
using statistic::io::parse_numbers;

const std::filesystem::path kDataDir { STATISTIC_TEST_DATA_DIR };

} // namespace

TEST(csv_irregular_rows_are_flattened)
{
    // 3, 1, 0 (empty), 2 and 1 columns -> one flat vector, in reading order.
    const auto r = parse_numbers("1.0 5.5 8.4\n2.3\n\n4.5 56.6\n0.0\n");
    const std::vector<double> expected { 1.0, 5.5, 8.4, 2.3, 4.5, 56.6, 0.0 };
    CHECK(r.values == expected);
    CHECK(r.skipped_tokens == 0);
}

TEST(csv_mixed_separators)
{
    const auto r = parse_numbers("1,2;3\t4   5 ,; 6");
    const std::vector<double> expected { 1, 2, 3, 4, 5, 6 };
    CHECK(r.values == expected);
}

TEST(csv_skips_and_counts_garbage)
{
    // Each of these 8 tokens must be rejected as a whole.
    const auto r = parse_numbers("abc 1.0x 0x1F nan inf - 1e999 +-3 7");
    CHECK(r.values == std::vector<double> { 7.0 });
    CHECK(r.skipped_tokens == 8);
}

TEST(csv_number_formats)
{
    const auto r = parse_numbers("+1.5 -2 .5 5. 1e3 -2.5E-1");
    const std::vector<double> expected { 1.5, -2.0, 0.5, 5.0, 1000.0, -0.25 };
    CHECK(r.values == expected);
    CHECK(r.skipped_tokens == 0);
}

TEST(csv_crlf_bom_and_no_trailing_newline)
{
    const auto r = parse_numbers("\xEF\xBB\xBF"
                                 "1.5\r\n2.5\r\n3.5"); // BOM + CRLF, no final \n
    const std::vector<double> expected { 1.5, 2.5, 3.5 };
    CHECK(r.values == expected);
    CHECK(r.skipped_tokens == 0);
}

TEST(csv_embedded_nul_and_binary_junk)
{
    // Explicit length (11) because the literal contains an embedded NUL.
    const std::string text("1.0 \0\x01\xFF 2.0", 11);
    const auto r = parse_numbers(text);
    const std::vector<double> expected { 1.0, 2.0 };
    CHECK(r.values == expected);
    CHECK(r.skipped_tokens == 1);
}

TEST(csv_empty_text)
{
    const auto r = parse_numbers("");
    CHECK(r.values.empty());
    CHECK(r.skipped_tokens == 0);
}

TEST(csv_missing_file)
{
    const auto r = statistic::io::read_numbers(kDataDir / "does_not_exist.csv");
    CHECK(!r.has_value());
    CHECK(r.error() == statistic::io::CsvError::FileNotFound);
}

TEST(csv_directory_is_not_a_file)
{
    const auto r = statistic::io::read_numbers(kDataDir);
    CHECK(!r.has_value());
    CHECK(r.error() == statistic::io::CsvError::FileNotFound);
}

TEST(csv_file_without_numbers)
{
    const auto r = statistic::io::read_numbers(kDataDir / "garbage_only.csv");
    CHECK(!r.has_value());
    CHECK(r.error() == statistic::io::CsvError::NoNumericData);
}

TEST(csv_messy_file_end_to_end)
{
    // messy.csv holds the 10 numbers of the original sample plus 13 junk tokens.
    const auto r = statistic::io::read_numbers(kDataDir / "messy.csv");
    CHECK(r.has_value());
    if (!r)
        return;

    const std::vector<double> expected { 1.0, 5.5, 8.4, 2.3, 4.5, 56.6, 1000.2, 0.0, 1.2, 1.0 };
    CHECK(r->values == expected);
    CHECK(r->skipped_tokens == 13);

    // Reference values computed independently with Python:
    //   mean 108.07, median 3.4, population variance 88694.2141, stddev 297.81573...
    const auto s = statistic::summarize(r->values);
    CHECK(s.has_value());
    if (!s)
        return;
    CHECK_NEAR(s->mean, 108.07, kEps);
    CHECK_NEAR(s->median, 3.4, kEps);
    CHECK_NEAR(s->variance, 88694.2141, 1e-4);
    CHECK_NEAR(s->stddev, 297.81574, 1e-4);
}
