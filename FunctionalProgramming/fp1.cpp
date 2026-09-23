/*
 * fp1.cpp
 *
 * Functional Programming
 *
 * Build:
 *
 * g++ -std=c++26 fp1.cpp -o test1
 */

#include <algorithm>
#include <cstdlib>
#include <expected>
#include <functional>
#include <print>
#include <ranges>
#include <string>
#include <vector>

namespace {

template <typename T>
using Result = std::expected<T, std::string>;

auto validate_percent(int percent) -> Result<int>
{
    return percent < 0
        ? Result<int>(std::unexpected("Error: The percent cannot be negative."))
        : Result<int>(percent);
}

auto compute(const std::vector<float>& values, float limit, int percent) -> std::vector<float>
{
    auto processed = values
        | std::views::transform([limit, percent](float v) {
              return v > limit ? v * static_cast<float>(percent) / 100.00f : 0.0f;
          });

    return std::vector<float>(processed.begin(), processed.end());
}

// Monadic composition: validate first, then transform the success value into the computed result
auto calc(const std::vector<float>& values, float limit, int percent) -> Result<std::vector<float>>
{
    return validate_percent(percent)
        .transform([&values, limit](int p) { return compute(values, limit, p); });
}

void show(const std::vector<float>& values, const std::vector<float>& calculated)
{
    std::println("+------------+------------+");
    std::println("|   Sale     |    Bonus   |");
    std::println("+------------+------------+");

    const size_t max_size = std::max(values.size(), calculated.size());

    auto pad = [max_size](const std::vector<float>& v) {
        return std::views::concat(v, std::views::repeat(0.0f) | std::views::take(max_size - v.size()));
    };

    for (auto [a, c] : std::views::zip(pad(values), pad(calculated))) {
        std::println("| {:>10.2f} | {:>10.2f} |", a, c);
    }

    std::println("+------------+------------+");
}

auto report(const std::string& title, const std::vector<float>& sales_values, float limit, int percent) -> bool
{
    std::println("");
    std::println("=== {} (limit={:.2f}, percent={}) ===", title, limit, percent);

    auto outcome = calc(sales_values, limit, percent)
                       .transform([&sales_values](const std::vector<float>& calculated) {
                           show(sales_values, calculated);
                           return calculated;
                       })
                       .or_else([](const std::string& error) -> Result<std::vector<float>> {
                           std::println("{}", error);
                           return std::unexpected(error);
                       });

    return outcome.has_value();
}

}

auto main() -> int
{
    const std::vector<float> sales_values { 5000, 2580, 9000.65f, 18000, 3500, 6789, 200 };
    const std::vector<float> small_values { 10, 20, 30 };
    const std::vector<float> empty_values {};

    const std::vector<bool> results {
        report("Standard sales, 5% bonus", sales_values, 3000.0f, 5),
        report("Higher limit, 10% bonus", sales_values, 10000.0f, 10),
        report("Limit excludes everything", sales_values, 20000.0f, 5),
        report("Small dataset, 50% bonus", small_values, 15.0f, 50),
        report("Empty dataset", empty_values, 100.0f, 5),
        report("Invalid percent (error path)", sales_values, 3000.0f, -5),
    };

    const bool all_ok = std::ranges::all_of(results, std::identity {});

    std::println("");
    std::println("Summary: {}/{} scenarios succeeded.", std::ranges::count(results, true), results.size());

    return all_ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
