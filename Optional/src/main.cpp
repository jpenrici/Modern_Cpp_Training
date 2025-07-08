#include <cctype>
#include <iostream>
#include <optional>
#include <print>
#include <string>
#include <vector>

struct Quiz {
  std::string question;
  std::string answer;
};

std::vector<Quiz> questions{{"Lives in water?", "fish"},
                            {"Has seven lives?", "cat"},
                            {"King of the jungle?", "lion"}};

auto toLower(const std::string &str) -> std::string {
  std::string lowerStr = str;
  for (char &c : lowerStr) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return lowerStr;
}

auto checkAnswer(std::string answer, size_t index) -> std::optional<bool> {
  if (answer.empty()) {
    return std::nullopt;
  }
  return toLower(answer) == toLower(questions.at(index).answer);
}

auto main() -> int {
  unsigned score{0};
  unsigned max = questions.size();

  std::string input;

  std::println("--- Quiz ---");

  for (size_t i = 0; i < max; i++) {
    std::println("{}", questions.at(i).question);
    std::getline(std::cin, input);

    std::optional<bool> result = checkAnswer(input, i);
    if (result.has_value()) {
      if (result.value()) {
        std::println("Congratulations! Correct answer!");
        score++;
      } else {
        std::println(
            "What a shame! Incorrect answer. The correct answer was: {}",
            questions.at(i).answer);
      }
    } else {
      std::println("You didn't provide an answer. Skipping this question.");
    }
    std::println();
  }

  std::println("Score: {}/{}", score, max);
  std::println("--- Quiz Finished ---");

  return 0;
}
