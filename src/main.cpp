#include <exception>
#include <iostream>
#include <print>
#include <variant>

import rangequery.config;
import rangequery.structures.factory;

namespace {
constexpr int kInvalidInputExitCode = 2;
constexpr int kOkExitCode = 0;

template <rq::structures::OrderStatisticSet Set>
int run_commands(Set& structure) {
  char command{};
  while (std::cin >> command) {
    switch (command) {
      case 'k': {
        int value{};
        if (!(std::cin >> value)) {
          std::println(stderr, "invalid k command: expected an integer");
          return kInvalidInputExitCode;
        }
        structure.insert(value);
        break;
      }
      case 'q': {
        int left{}, right{};
        if (!(std::cin >> left >> right)) {
          std::println(stderr, "invalid q command: expected two integers");
          return kInvalidInputExitCode;
        }
        std::println("{}", structure.count_in_open_closed_range(left, right));
        break;
      }
      case 'm': {
        long long index{};
        if (!(std::cin >> index) || index <= 0) {
          std::println(stderr, "invalid m command: expected a positive index");
          return kInvalidInputExitCode;
        }
        const auto result = structure.select(static_cast<unsigned long long>(index));
        if (!result.has_value()) {
          std::println(stderr, "m index is outside the set");
          return kInvalidInputExitCode;
        }
        std::println("{}", *result);
        break;
      }
      case 'n': {
        int value{};
        if (!(std::cin >> value)) {
          std::println(stderr, "invalid n command: expected an integer");
          return kInvalidInputExitCode;
        }
        std::println("{}", structure.count_less(value));
        break;
      }
      default:
        std::println(stderr, "unknown command: {}", command);
        return kInvalidInputExitCode;
    }
  }

  if (!std::cin.eof()) {
    std::println(stderr, "malformed input");
    return kInvalidInputExitCode;
  }
  return kOkExitCode;
}
}  // namespace

int main(int argc, char* argv[]) try {
  const auto settings = rq::read_command_line(argc, argv);

  if (!settings) return settings.error();

  auto structure = rq::structures::make_structure(settings->structure);
  if (!structure) {
    std::println(stderr, "{}", structure.error());
    return kInvalidInputExitCode;
  }

  return std::visit([](auto& selected) { return run_commands(selected); }, *structure);
} catch (const std::exception& error) {
  std::println(stderr, "rangequery: {}", error.what());
  return kInvalidInputExitCode;
} catch (...) {
  std::println(stderr, "rangequery: unknown exception");
  return kInvalidInputExitCode;
}
