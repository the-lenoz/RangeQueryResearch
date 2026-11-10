#include <gtest/gtest.h>
#include <array>
#include <expected>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

import rangequery.config;

namespace {

template <typename Parser>
auto read_arguments(Parser parser, std::initializer_list<const char*> arguments) {
  std::vector<char*> argv{const_cast<char*>("rangequery")};
  for (const char* argument : arguments) argv.push_back(const_cast<char*>(argument));
  return parser(static_cast<int>(argv.size()), argv.data());
}

template <typename Parser>
void expect_cli_exit(Parser parser, std::initializer_list<const char*> arguments, int expected_code,
                     std::string_view fragment) {
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  const auto result = read_arguments(parser, arguments);
  const auto output = testing::internal::GetCapturedStdout();
  const auto diagnostic = testing::internal::GetCapturedStderr();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), expected_code);
  if (expected_code == 0) {
    EXPECT_TRUE(diagnostic.empty());
    EXPECT_NE(output.find("[OPTIONS]"), std::string::npos);
    EXPECT_NE(output.find(fragment), std::string::npos);
  } else {
    EXPECT_TRUE(output.empty());
    EXPECT_NE(diagnostic.find(fragment), std::string::npos);
  }
}

}  // namespace

TEST(CommandLineConfiguration, DefaultsToOrderedVector) {
  const auto settings = read_arguments(rq::read_command_line, {});
  ASSERT_TRUE(settings.has_value());
  EXPECT_EQ(settings->structure, rq::StructureKind::ordered_vector);
}

TEST(CommandLineConfiguration, ParsesEverySupportedStructureInBothExecutables) {
  const std::array cases{
      std::pair{"vector", rq::StructureKind::ordered_vector},
      std::pair{"set", rq::StructureKind::std_set},
      std::pair{"avl", rq::StructureKind::avl},
      std::pair{"treap", rq::StructureKind::treap},
  };
  for (const auto& [name, expected] : cases) {
    SCOPED_TRACE(name);
    const auto settings = read_arguments(rq::read_command_line, {"--structure", name});
    ASSERT_TRUE(settings.has_value());
    EXPECT_EQ(settings->structure, expected);
    const auto benchmark =
        read_arguments(rq::read_benchmark_command_line, {"--structure", name, "--mode", "queries"});
    ASSERT_TRUE(benchmark.has_value());
    EXPECT_EQ(benchmark->structure, expected);
    EXPECT_EQ(benchmark->mode, rq::BenchmarkMode::queries);
  }
}

TEST(CommandLineConfiguration, RecognizesBothLibraryHelpOptions) {
  for (const char* option : {"-h", "--help"}) {
    expect_cli_exit(rq::read_command_line, {option}, 0, "--structure");
    expect_cli_exit(rq::read_benchmark_command_line, {option}, 0, "--setup-count");
  }
}

TEST(CommandLineConfiguration, AcceptsEqualsSyntax) {
  const auto settings = read_arguments(rq::read_command_line, {"--structure=avl"});
  ASSERT_TRUE(settings.has_value());
  EXPECT_EQ(settings->structure, rq::StructureKind::avl);
  const auto benchmark = read_arguments(rq::read_benchmark_command_line,
                                        {"--structure=treap", "--mode=dynamic", "--setup-count=2"});
  ASSERT_TRUE(benchmark.has_value());
  EXPECT_EQ(benchmark->structure, rq::StructureKind::treap);
  EXPECT_EQ(benchmark->mode, rq::BenchmarkMode::dynamic);
  EXPECT_EQ(benchmark->setup_count, 2U);
}

TEST(CommandLineConfiguration, RejectsInvalidAndRepeatedArguments) {
  for (const auto arguments : {std::initializer_list<const char*>{"--structure", "binary_tree"},
                               {"--structure"},
                               {"--structure", "vector", "--structure", "avl"}}) {
    expect_cli_exit(rq::read_command_line, arguments, 2, "--structure");
  }
  expect_cli_exit(rq::read_command_line, {"--unknown"}, 2, "--unknown");
}

TEST(CommandLineConfiguration, RequiresBenchmarkOptionsAndValidMode) {
  expect_cli_exit(rq::read_benchmark_command_line, {}, 2, "--structure");
  expect_cli_exit(rq::read_benchmark_command_line, {"--structure", "avl"}, 2, "--mode");
  expect_cli_exit(rq::read_benchmark_command_line, {"--structure", "avl", "--mode"}, 2, "--mode");
  expect_cli_exit(rq::read_benchmark_command_line, {"--structure", "avl", "--mode", "wrong"}, 2,
                  "--mode");
}

TEST(CommandLineConfiguration, RequiresSetupCountOnlyInDynamicMode) {
  expect_cli_exit(rq::read_benchmark_command_line, {"--structure", "avl", "--mode", "dynamic"}, 2,
                  "--setup-count");
  for (const char* mode : {"queries", "insert"}) {
    const auto settings =
        read_arguments(rq::read_benchmark_command_line, {"--structure", "avl", "--mode", mode});
    ASSERT_TRUE(settings.has_value());
    EXPECT_EQ(settings->mode, std::string_view{mode} == "queries" ? rq::BenchmarkMode::queries
                                                                  : rq::BenchmarkMode::insert);
    expect_cli_exit(rq::read_benchmark_command_line,
                    {"--structure", "avl", "--mode", mode, "--setup-count", "1"}, 2,
                    "--setup-count");
  }
}

TEST(CommandLineConfiguration, RejectsInvalidSetupCounts) {
  for (const char* count : {"0", "-1", "invalid", "1.5", "1x", "18446744073709551616"}) {
    SCOPED_TRACE(count);
    expect_cli_exit(rq::read_benchmark_command_line,
                    {"--structure", "avl", "--mode", "dynamic", "--setup-count", count}, 2,
                    "--setup-count");
  }
}

TEST(CommandLineConfiguration, ConvertsEveryStructureKindToText) {
  EXPECT_EQ(rq::to_string(rq::StructureKind::ordered_vector), std::string_view{"vector"});
  EXPECT_EQ(rq::to_string(rq::StructureKind::std_set), std::string_view{"set"});
  EXPECT_EQ(rq::to_string(rq::StructureKind::avl), std::string_view{"avl"});
  EXPECT_EQ(rq::to_string(rq::StructureKind::treap), std::string_view{"treap"});
  EXPECT_EQ(rq::to_string(static_cast<rq::StructureKind>(-1)), std::string_view{"unknown"});
}
