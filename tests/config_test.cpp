#include <gtest/gtest.h>
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


TEST(CommandLineConfiguration, RejectsInvalidAndRepeatedArguments) {
  for (const auto arguments : {std::initializer_list<const char*>{"--structure", "binary_tree"},
                               {"--structure"},
                               {"--structure", "vector", "--structure", "avl"}}) {
    expect_cli_exit(rq::read_command_line, arguments, 2, "--structure");
  }
  expect_cli_exit(rq::read_command_line, {"--unknown"}, 2, "--unknown");
}

TEST(CommandLineConfiguration, ConvertsEveryStructureKindToText) {
  EXPECT_EQ(rq::to_string(rq::StructureKind::ordered_vector), std::string_view{"vector"});
  EXPECT_EQ(rq::to_string(rq::StructureKind::std_set), std::string_view{"set"});
  EXPECT_EQ(rq::to_string(rq::StructureKind::avl), std::string_view{"avl"});
  EXPECT_EQ(rq::to_string(rq::StructureKind::treap), std::string_view{"treap"});
  EXPECT_EQ(rq::to_string(static_cast<rq::StructureKind>(-1)), std::string_view{"unknown"});
}
