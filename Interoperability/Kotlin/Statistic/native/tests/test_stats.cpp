// Statistic/native/tests/test_stats.cpp

/**
 * @file test_stats.cpp
 * @brief Unit tests of the statistics core (src/core/stats.hpp).
 */
#include "minitest.hpp"

#include "core/stats.hpp"

#include <limits>
#include <vector>

namespace {

constexpr double kEps = 1e-9;
constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kInf = std::numeric_limits<double>::infinity();

} // namespace

TEST(stats_single_element)
{
    const std::vector<double> v { 42.0 };
    const auto s = statistic::summarize(v);
    CHECK(s.has_value());
    CHECK_NEAR(s->mean, 42.0, kEps);
    CHECK_NEAR(s->median, 42.0, kEps);
    CHECK_NEAR(s->variance, 0.0, kEps);
    CHECK_NEAR(s->stddev, 0.0, kEps);
}

TEST(stats_odd_count_unsorted)
{
    // sorted: 1 2 3 4 9 -> median 3, mean 3.8
    const std::vector<double> v { 9.0, 1.0, 4.0, 3.0, 2.0 };
    CHECK_NEAR(*statistic::mean(v), 3.8, kEps);
    CHECK_NEAR(*statistic::median(v), 3.0, kEps);
}

TEST(stats_even_count_median_is_midpoint)
{
    const std::vector<double> v { 8.0, 2.0, 4.0, 6.0 }; // sorted: 2 4 6 8
    CHECK_NEAR(*statistic::median(v), 5.0, kEps);
}

TEST(stats_population_variance)
{
    // Classic example: mean 5, population variance 4, stddev 2.
    const std::vector<double> v { 2, 4, 4, 4, 5, 5, 7, 9 };
    const auto s = statistic::summarize(v);
    CHECK(s.has_value());
    CHECK_NEAR(s->mean, 5.0, kEps);
    CHECK_NEAR(s->variance, 4.0, kEps);
    CHECK_NEAR(s->stddev, 2.0, kEps);
    CHECK_NEAR(*statistic::std_dev(v), 2.0, kEps);
}

TEST(stats_negative_and_constant_values)
{
    const std::vector<double> neg { -3.0, -1.0, -2.0 };
    CHECK_NEAR(*statistic::mean(neg), -2.0, kEps);
    CHECK_NEAR(*statistic::median(neg), -2.0, kEps);

    const std::vector<double> flat(100, 7.5);
    CHECK_NEAR(*statistic::variance(flat), 0.0, kEps);
}

TEST(stats_does_not_modify_input)
{
    const std::vector<double> v { 3.0, 1.0, 2.0 };
    const auto copy = v;
    (void)statistic::median(v); // median sorts a private copy
    CHECK(v == copy);
}

TEST(stats_rejects_empty_input)
{
    const std::vector<double> v;
    const auto s = statistic::summarize(v);
    CHECK(!s.has_value());
    CHECK(s.error() == statistic::StatsError::EmptyInput);
    CHECK(!statistic::mean(v).has_value());
    CHECK(!statistic::median(v).has_value());
}

TEST(stats_rejects_nan_and_infinity)
{
    const std::vector<double> with_nan { 1.0, kNaN, 3.0 };
    const std::vector<double> with_inf { 1.0, kInf };
    CHECK(statistic::summarize(with_nan).error() == statistic::StatsError::NonFiniteValue);
    CHECK(statistic::median(with_inf).error() == statistic::StatsError::NonFiniteValue);
}

TEST(stats_detects_overflow)
{
    // The sum of these two finite values does not fit in a double.
    const double big = std::numeric_limits<double>::max();
    const std::vector<double> v { big, big };
    const auto s = statistic::summarize(v);
    CHECK(!s.has_value());
    CHECK(s.error() == statistic::StatsError::NumericOverflow);
}

TEST(stats_median_is_overflow_safe)
{
    // (a + b) / 2 would overflow; std::midpoint must not.
    const double big = std::numeric_limits<double>::max();
    const std::vector<double> v { big, big };
    const auto m = statistic::median(v);
    CHECK(m.has_value());
    CHECK(*m == big);
}
