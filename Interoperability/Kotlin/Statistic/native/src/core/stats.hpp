// Statistic/native/src/core/stats.cpp

/**
 * @file stats.hpp
 * @brief Internal C++23 statistics core. NOT part of the public API.
 *
 * All functions treat the input as ONE flat vector of numbers.
 * Errors are reported with std::expected instead of exceptions.
 */
#pragma once

#include <expected>
#include <span>
#include <string_view>

namespace statistic {

/// Reasons why a statistic cannot be computed.
enum class StatsError {
    EmptyInput, ///< The input has no elements.
    NonFiniteValue, ///< The input contains NaN or +/-infinity.
    NumericOverflow ///< An intermediate or final value is not finite.
};

/// Name of an error, useful for logs and test output.
[[nodiscard]] constexpr auto to_string(StatsError error) noexcept -> std::string_view
{
    switch (error) {
    case StatsError::EmptyInput:
        return "EmptyInput";
    case StatsError::NonFiniteValue:
        return "NonFiniteValue";
    case StatsError::NumericOverflow:
        return "NumericOverflow";
    }
    return "Unknown";
}

/// Result of summarize().
struct Summary {
    double mean {};
    double median {};
    double variance {};
    double stddev {};
};

template <typename T>
using Result = std::expected<T, StatsError>;

using Data = std::span<const double>;

/// Arithmetic mean.
[[nodiscard]] auto mean(Data data) -> Result<double>;

/// Median. For an even count it is the midpoint of the two central values.
[[nodiscard]] auto median(Data data) -> Result<double>;

/// Population variance (divides by n). A single element has variance 0.
[[nodiscard]] auto variance(Data data) -> Result<double>;

/// Population standard deviation, sqrt(variance).
[[nodiscard]] auto std_dev(Data data) -> Result<double>;

/// Computes everything in one call; validates the input only once.
[[nodiscard]] auto summarize(Data data) -> Result<Summary>;

} // namespace statistic
