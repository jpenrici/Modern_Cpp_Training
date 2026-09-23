/*
 * fp3.cpp
 *
 * Functional Programming
 *
 * Build:
 *
 * g++ -std=c++26 fp3.cpp -o test3
 */

#include <algorithm>
#include <cmath>
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

// --- Pure statistical building blocks --------------------------------------

auto sum(const std::vector<double>& values) -> double
{
    return std::ranges::fold_left(values, 0.0, std::plus {});
}

auto mean(const std::vector<double>& values) -> Result<double>
{
    if (values.empty()) {
        return std::unexpected("Error: cannot compute the mean of an empty dataset.");
    }
    return sum(values) / static_cast<double>(values.size());
}

// Population variance: average of the squared deviations from the mean.
auto variance(const std::vector<double>& values) -> Result<double>
{
    return mean(values).and_then([&values](double avg) -> Result<double> {
        auto squared_deviations = values
            | std::views::transform([avg](double v) { return (v - avg) * (v - avg); });

        return std::ranges::fold_left(squared_deviations, 0.0, std::plus {}) / static_cast<double>(values.size());
    });
}

auto stddev(const std::vector<double>& values) -> Result<double>
{
    return variance(values).transform([](double var) { return std::sqrt(var); });
}

// --- Shape-preserving transform: z-scores -----------------------------------
auto z_scores(const std::vector<double>& values) -> Result<std::vector<double>>
{
    return mean(values).and_then([&values](double avg) {
        return stddev(values).transform([&values, avg](double sd) {
            auto scores = values
                | std::views::transform([avg, sd](double v) { return sd > 0.0 ? (v - avg) / sd : 0.0; });
            return std::vector<double>(scores.begin(), scores.end());
        });
    });
}

// --- Shape-changing filter: outliers -----------------------------------------
auto outliers(const std::vector<double>& values, double threshold) -> Result<std::vector<double>>
{
    return z_scores(values).transform([&values, threshold](const std::vector<double>& zs) {
        auto flagged = std::views::zip(values, zs)
            | std::views::filter([threshold](const auto& pair) {
                  const auto& [value, z] = pair;
                  return std::abs(z) > threshold;
              })
            | std::views::transform([](const auto& pair) { return std::get<0>(pair); });

        return std::vector<double>(flagged.begin(), flagged.end());
    });
}

// --- Windowed computation: moving average -------------------------------------
auto moving_average(const std::vector<double>& values, size_t window) -> Result<std::vector<double>>
{
    if (window == 0 || window > values.size()) {
        return std::unexpected("Error: window size must be between 1 and the dataset size.");
    }

    auto averages = values
        | std::views::slide(window)
        | std::views::transform([window](auto win) {
              return std::ranges::fold_left(win, 0.0, std::plus {}) / static_cast<double>(window);
          });

    return std::vector<double>(averages.begin(), averages.end());
}

// --- A tiny helper to keep main() free of repeated if/else -------------------
template <typename T, typename F>
void print_result(const Result<T>& result, F on_success)
{
    if (result) {
        on_success(*result);
    } else {
        std::println("{}", result.error());
    }
}

}

auto main() -> int
{
    const std::vector<double> temperatures { 21.5, 22.0, 21.8, 35.0, 22.3, 21.9, 22.1, 5.0, 22.4, 22.0 };

    std::println("=== Descriptive statistics ===");
    print_result(mean(temperatures), [](double m) { std::println("Mean:    {:.3f}", m); });
    print_result(stddev(temperatures), [](double sd) { std::println("Std dev: {:.3f}", sd); });

    std::println("");
    std::println("=== Z-scores (shape-preserving transform) ===");
    print_result(z_scores(temperatures), [&temperatures](const std::vector<double>& zs) {
        for (auto [t, z] : std::views::zip(temperatures, zs)) {
            std::println("  {:>6.2f} -> z = {:>6.2f}", t, z);
        }
    });

    std::println("");
    std::println("=== Outliers, |z| > 1.5 (shape-changing filter) ===");
    print_result(outliers(temperatures, 1.5), [](const std::vector<double>& out) {
        for (double v : out) {
            std::println("  {:.2f}", v);
        }
    });

    std::println("");
    std::println("=== 3-point moving average (views::slide) ===");
    print_result(moving_average(temperatures, 3), [](const std::vector<double>& avgs) {
        for (double a : avgs) {
            std::println("  {:.3f}", a);
        }
    });

    std::println("");
    std::println("=== Error path: empty dataset ===");
    const std::vector<double> empty_values {};
    print_result(mean(empty_values), [](double) { /* unreachable on error path */ });

    return EXIT_SUCCESS;
}
