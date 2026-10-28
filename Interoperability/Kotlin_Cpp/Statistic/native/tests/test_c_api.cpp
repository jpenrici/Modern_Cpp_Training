// Statistic/native/tests/test_c_api.cpp

/**
 * @file test_c_api.cpp
 * @brief Tests of the exported C boundary, exactly as Kotlin/Native will call it.
 *
 * This file includes ONLY the public header, never the C++ internals.
 */
#include "minitest.hpp"

#include "statistic/stats_c_api.h"

#include <cstring>
#include <limits>

namespace {
constexpr double kEps = 1e-9;
}

TEST(capi_success)
{
    const double data[] = { 2, 4, 4, 4, 5, 5, 7, 9 };
    StatsResult r {};
    CHECK(stats_compute(data, 8, &r) == STATS_OK);
    CHECK_NEAR(r.mean, 5.0, kEps);
    CHECK_NEAR(r.median, 4.5, kEps);
    CHECK_NEAR(r.variance, 4.0, kEps);
    CHECK_NEAR(r.stddev, 2.0, kEps);
}

TEST(capi_null_arguments)
{
    const double data[] = { 1.0 };
    StatsResult r {};
    CHECK(stats_compute(nullptr, 1, &r) == STATS_ERR_NULL_ARGUMENT);
    CHECK(stats_compute(data, 1, nullptr) == STATS_ERR_NULL_ARGUMENT);
}

TEST(capi_invalid_sizes)
{
    const double data[] = { 1.0 };
    StatsResult r {};
    CHECK(stats_compute(data, 0, &r) == STATS_ERR_EMPTY_INPUT);
    CHECK(stats_compute(data, -1, &r) == STATS_ERR_INVALID_SIZE);
}

TEST(capi_non_finite_input)
{
    const double data[] = { 1.0, std::numeric_limits<double>::quiet_NaN() };
    StatsResult r {};
    CHECK(stats_compute(data, 2, &r) == STATS_ERR_NON_FINITE);
}

TEST(capi_overflow)
{
    const double big = std::numeric_limits<double>::max();
    const double data[] = { big, big };
    StatsResult r {};
    CHECK(stats_compute(data, 2, &r) == STATS_ERR_OVERFLOW);
}

TEST(capi_output_is_zeroed_on_error)
{
    const double data[] = { 1.0 };
    StatsResult r { 1.0, 2.0, 3.0, 4.0 }; // pre-filled with junk
    CHECK(stats_compute(data, 0, &r) == STATS_ERR_EMPTY_INPUT);
    CHECK(r.mean == 0.0 && r.median == 0.0 && r.variance == 0.0 && r.stddev == 0.0);
}

TEST(capi_struct_layout_matches_kotlin_expectation)
{
    // Kotlin/Native cinterop reads this as 4 consecutive doubles.
    static_assert(sizeof(StatsResult) == 4 * sizeof(double), "StatsResult must have no padding");
    static_assert(offsetof(StatsResult, stddev) == 3 * sizeof(double));
    CHECK(true);
}

TEST(capi_status_messages_are_never_null)
{
    for (int s = STATS_OK; s <= STATS_ERR_INTERNAL; ++s) {
        const char* msg = stats_status_message(static_cast<StatsStatus>(s));
        CHECK(msg != nullptr && std::strlen(msg) > 0);
    }
    CHECK(stats_status_message(static_cast<StatsStatus>(STATS_ERR_INTERNAL + 1)) != nullptr);
}
