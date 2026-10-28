// Statistic/native/src/c_api/stats_c_api.cpp

/**
 * @file stats_c_api.cpp
 * @brief Thin C adapter over the C++ core. Exceptions never cross this boundary.
 */
#include "statistic/stats_c_api.h"

#include "core/stats.hpp"

#include <cstddef>
#include <span>

namespace {

auto to_c_status(statistic::StatsError error) noexcept -> StatsStatus
{
    switch (error) {
    case statistic::StatsError::EmptyInput:
        return STATS_ERR_EMPTY_INPUT;
    case statistic::StatsError::NonFiniteValue:
        return STATS_ERR_NON_FINITE;
    case statistic::StatsError::NumericOverflow:
        return STATS_ERR_OVERFLOW;
    }
    return STATS_ERR_INTERNAL;
}

} // namespace

extern "C" {

StatsStatus stats_compute(const double* data, int64_t size, StatsResult* out)
{
    if (out == nullptr)
        return STATS_ERR_NULL_ARGUMENT;
    *out = StatsResult {}; // never leave the caller with garbage

    if (data == nullptr)
        return STATS_ERR_NULL_ARGUMENT;
    if (size < 0)
        return STATS_ERR_INVALID_SIZE;

    try {
        const std::span<const double> view { data, static_cast<std::size_t>(size) };
        const auto summary = statistic::summarize(view);
        if (!summary)
            return to_c_status(summary.error());

        *out = StatsResult { summary->mean, summary->median, summary->variance, summary->stddev };
        return STATS_OK;
    } catch (...) { // e.g. std::bad_alloc: must never unwind into Kotlin/JVM
        return STATS_ERR_INTERNAL;
    }
}

const char* stats_status_message(StatsStatus status)
{
    switch (status) {
    case STATS_OK:
        return "ok";
    case STATS_ERR_NULL_ARGUMENT:
        return "null argument";
    case STATS_ERR_INVALID_SIZE:
        return "negative size";
    case STATS_ERR_EMPTY_INPUT:
        return "empty input";
    case STATS_ERR_NON_FINITE:
        return "input contains NaN or infinity";
    case STATS_ERR_OVERFLOW:
        return "numeric overflow";
    case STATS_ERR_INTERNAL:
        return "internal error";
    }
    return "unknown status";
}

} // extern "C"
