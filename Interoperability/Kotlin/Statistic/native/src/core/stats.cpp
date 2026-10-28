// Statistic/native/src/core/stats.hpp

/**
 * @file stats.cpp
 * @brief Implementation of the statistics core (see stats.hpp).
 */
#include "core/stats.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <numeric>
#include <vector>

namespace statistic {
namespace {

    using Unexpected = std::unexpected<StatsError>;

    /// Rejects empty input and any NaN/infinity (they would break sorting and sums).
    auto validate(Data data) -> Result<void>
    {
        if (data.empty())
            return Unexpected { StatsError::EmptyInput };
        if (!std::ranges::all_of(data, [](double v) { return std::isfinite(v); }))
            return Unexpected { StatsError::NonFiniteValue };
        return {};
    }

    /// Turns a non-finite computed value into NumericOverflow.
    auto ensure_finite(double value) -> Result<double>
    {
        if (!std::isfinite(value))
            return Unexpected { StatsError::NumericOverflow };
        return value;
    }

    // ---- "unchecked" kernels: the caller guarantees non-empty, finite input ----

    auto mean_unchecked(Data data) -> double
    {
        const double sum = std::ranges::fold_left(data, 0.0, std::plus<> {});
        return sum / static_cast<double>(data.size());
    }

    auto median_unchecked(Data data) -> double
    {
        std::vector<double> copy(data.begin(), data.end());
        const auto mid = copy.begin() + static_cast<std::ptrdiff_t>(copy.size() / 2);

        // After nth_element, *mid is the element that would sit at index n/2
        // in the sorted sequence, and everything before it is <= *mid.
        std::ranges::nth_element(copy, mid);
        const double upper = *mid;
        if (copy.size() % 2 != 0)
            return upper;

        // Even count: the lower central value is the maximum of the left half.
        const double lower = *std::ranges::max_element(copy.begin(), mid);
        return std::midpoint(lower, upper); // overflow-safe (a+b)/2
    }

    auto variance_unchecked(Data data, double mean) -> double
    {
        const double sum_sq = std::ranges::fold_left(data, 0.0, [mean](double acc, double x) {
            const double d = x - mean;
            return acc + d * d;
        });
        return sum_sq / static_cast<double>(data.size());
    }

} // namespace

auto mean(Data data) -> Result<double>
{
    return validate(data).and_then([&] { return ensure_finite(mean_unchecked(data)); });
}

auto median(Data data) -> Result<double>
{
    return validate(data).and_then([&] { return ensure_finite(median_unchecked(data)); });
}

auto variance(Data data) -> Result<double>
{
    return validate(data)
        .and_then([&] { return ensure_finite(mean_unchecked(data)); })
        .and_then([&](double m) { return ensure_finite(variance_unchecked(data, m)); });
}

auto std_dev(Data data) -> Result<double>
{
    return variance(data).transform([](double v) { return std::sqrt(v); });
}

auto summarize(Data data) -> Result<Summary>
{
    if (auto ok = validate(data); !ok)
        return Unexpected { ok.error() };

    Summary s;

    const auto m = ensure_finite(mean_unchecked(data));
    if (!m)
        return Unexpected { m.error() };
    s.mean = *m;

    const auto med = ensure_finite(median_unchecked(data));
    if (!med)
        return Unexpected { med.error() };
    s.median = *med;

    const auto var = ensure_finite(variance_unchecked(data, s.mean));
    if (!var)
        return Unexpected { var.error() };
    s.variance = *var;

    s.stddev = std::sqrt(s.variance);
    return s;
}

} // namespace statistic
