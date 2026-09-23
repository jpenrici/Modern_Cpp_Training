/*
 * fp2.cpp
 *
 * Functional Programming
 *
 * Build:
 *
 * g++ -std=c++26 fp2.cpp -o test2
 */

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <optional>
#include <print>
#include <ranges>
#include <string>
#include <vector>

namespace {

// --- Pure functions ---------------------------------------------------
// A pure function only depends on its arguments and has no observable
// side effects: same input always produces the same output.

auto to_upper(std::string word) -> std::string
{
    std::ranges::transform(word, word.begin(), [](unsigned char c) { return std::toupper(c); });
    return word;
}

auto add_exclamation(std::string word) -> std::string
{
    return word + "!";
}

auto word_length(const std::string& word) -> size_t
{
    return word.size();
}

// --- Function composition ----------------------------------------------
// compose(f, g)(x) == f(g(x))
// Turns two small, single-purpose functions into one -- a classic FP
// building block, similar to what a pipe/chain of `views` does for ranges,
// but for plain functions instead of ranges.
template <typename F, typename G>
auto compose(F f, G g)
{
    return [f, g](auto&& x) { return f(g(std::forward<decltype(x)>(x))); };
}

// --- std::optional as a small monad -------------------------------------
// Returns the longest word, or std::nullopt if there isn't one.
// No magic sentinel values (like an empty string or -1) and no exceptions
// for what is really just "no answer" -- the type itself says so.
auto longest_word(const std::vector<std::string>& words) -> std::optional<std::string>
{
    if (words.empty()) {
        return std::nullopt;
    }

    return *std::ranges::max_element(words, std::ranges::less {}, word_length);
}

}

auto main() -> int
{
    const std::vector<std::string> words { "functional", "c++26", "is", "fun", "ranges", "monad" };

    // --- fold: reduce a range to a single value --------------------------
    // std::ranges::fold_left(range, init, op) is the range-based sibling of
    // std::accumulate: it combines every element with an accumulator using
    // a binary operation, left to right.
    const size_t total_chars = std::ranges::fold_left(
        words | std::views::transform(word_length), size_t { 0 }, std::plus {});

    std::println("Total characters across {} words: {}", words.size(), total_chars);

    // --- composition in action -------------------------------------------
    // shout = to_upper . add_exclamation, i.e. shout(w) == to_upper(add_exclamation(w))
    const auto shout = compose(to_upper, add_exclamation);

    const auto shouted = words
        | std::views::transform(shout)
        | std::ranges::to<std::vector<std::string>>();

    std::println("");
    std::println("Shouted words:");
    for (const auto& w : shouted) {
        std::println("  {}", w);
    }

    // --- optional monadic chain --------------------------------------------
    // .transform() runs 'shout' only if there is a value; .value_or() supplies
    // a fallback if there isn't. No manual "if (has_value())" needed.
    const std::string headline = longest_word(words)
                                     .transform(shout)
                                     .value_or("No words to shout about.");

    std::println("");
    std::println("Headline: {}", headline);

    // Same pipeline over an empty vector, to see the "nothing" path taken.
    const std::vector<std::string> empty_words {};
    const std::string empty_headline = longest_word(empty_words)
                                           .transform(shout)
                                           .value_or("No words to shout about.");

    std::println("Headline (empty input): {}", empty_headline);

    return EXIT_SUCCESS;
}
