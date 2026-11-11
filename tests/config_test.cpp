#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <expected>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

import rangequery.config;
import rangequery.benchmark.config;

namespace {
using namespace ::testing;

template <typename Settings>
struct CliResult {
  std::expected<Settings, int> settings;
  std::string output;
  std::string diagnostic;
};

template <typename Settings>
auto read_arguments(std::expected<Settings, int> (*parser)(int, char**),
                    std::vector<std::string> arguments) {
  char program_name[] = "rangequery";
  std::vector<char*> argv{program_name};
  std::transform(arguments.begin(), arguments.end(), std::back_inserter(argv),
                 [](std::string& argument) { return argument.data(); });
  internal::CaptureStdout();
  internal::CaptureStderr();
  auto settings = parser(static_cast<int>(argv.size()), argv.data());
  auto output = internal::GetCapturedStdout();
  auto diagnostic = internal::GetCapturedStderr();
  return CliResult<Settings>{std::move(settings), std::move(output), std::move(diagnostic)};
}

TEST(CommandLineConfiguration, DefaultsToOrderedVector) {
  const std::vector<std::string> arguments;

  const auto result = read_arguments(rq::read_command_line, arguments);

  ASSERT_TRUE(result.settings.has_value());
  EXPECT_EQ(result.settings->structure, rq::StructureKind::ordered_vector);
}

TEST(CommandLineConfiguration, SelectsRequestedStructure) {
  const std::vector<std::string> arguments{"--structure", "treap"};

  const auto result = read_arguments(rq::read_command_line, arguments);

  ASSERT_TRUE(result.settings.has_value());
  EXPECT_EQ(result.settings->structure, rq::StructureKind::treap);
}

TEST(CommandLineConfiguration, UnknownStructureRequestsErrorExit) {
  const std::vector<std::string> arguments{"--structure", "binary_tree"};

  const auto result = read_arguments(rq::read_command_line, arguments);

  ASSERT_FALSE(result.settings.has_value());
  EXPECT_EQ(result.settings.error(), 2);
  EXPECT_THAT(result.output, IsEmpty());
  EXPECT_THAT(result.diagnostic, HasSubstr("--structure"));
}

TEST(CommandLineConfiguration, HelpRequestsSuccessfulExit) {
  const std::vector<std::string> arguments{"--help"};

  const auto result = read_arguments(rq::read_command_line, arguments);

  ASSERT_FALSE(result.settings.has_value());
  EXPECT_EQ(result.settings.error(), 0);
  EXPECT_THAT(result.output, HasSubstr("--structure"));
  EXPECT_THAT(result.diagnostic, IsEmpty());
}

TEST(BenchmarkConfiguration, ReadsQuerySettingsWithoutSetup) {
  const std::vector<std::string> arguments{"--structure", "avl", "--mode", "queries"};

  const auto result = read_arguments(rq_benchmark::read_benchmark_command_line, arguments);

  ASSERT_TRUE(result.settings.has_value());
  EXPECT_EQ(result.settings->structure, rq::StructureKind::avl);
  EXPECT_EQ(result.settings->mode, rq_benchmark::BenchmarkMode::queries);
  EXPECT_EQ(result.settings->setup_count, 0U);
}

TEST(BenchmarkConfiguration, ReadsMinimalDynamicSetup) {
  const std::vector<std::string> arguments{"--structure", "treap",         "--mode",
                                           "dynamic",     "--setup-count", "1"};

  const auto result = read_arguments(rq_benchmark::read_benchmark_command_line, arguments);

  ASSERT_TRUE(result.settings.has_value());
  EXPECT_EQ(result.settings->mode, rq_benchmark::BenchmarkMode::dynamic);
  EXPECT_EQ(result.settings->setup_count, 1U);
}

struct BenchmarkErrorCase {
  std::string name;
  std::vector<std::string> arguments;
  std::string fragment;
};
void PrintTo(const BenchmarkErrorCase& value, std::ostream* os) {
  *os << value.name << ": " << PrintToString(value.arguments);
}
class BenchmarkConfigurationError : public TestWithParam<BenchmarkErrorCase> {};

TEST_P(BenchmarkConfigurationError, RejectsInvalidWorkloadSettings) {
  const auto param = GetParam();

  const auto result = read_arguments(rq_benchmark::read_benchmark_command_line, param.arguments);

  ASSERT_FALSE(result.settings.has_value());
  EXPECT_EQ(result.settings.error(), 2);
  EXPECT_THAT(result.diagnostic, HasSubstr(param.fragment));
}

INSTANTIATE_TEST_SUITE_P(
    WorkloadRules, BenchmarkConfigurationError,
    Values(BenchmarkErrorCase{"MissingMode", {"--structure", "avl"}, "--mode"},
           BenchmarkErrorCase{
               "MissingDynamicSetup", {"--structure", "avl", "--mode", "dynamic"}, "--setup-count"},
           BenchmarkErrorCase{"ZeroDynamicSetup",
                              {"--structure", "avl", "--mode", "dynamic", "--setup-count", "0"},
                              "--setup-count"},
           BenchmarkErrorCase{"SetupOutsideDynamic",
                              {"--structure", "avl", "--mode", "queries", "--setup-count", "1"},
                              "--setup-count"}),
    [](const auto& info) { return info.param.name; });
}  // namespace
