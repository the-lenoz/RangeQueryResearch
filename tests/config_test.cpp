#include <gtest/gtest.h>
#include <array>
#include <expected>
#include <string_view>
#include <vector>

import rangequery.config;

namespace {

std::expected<rq::Settings, std::string_view> read_arguments(
    std::initializer_list<const char*> arguments) {
  std::vector<char*> argv;
  argv.reserve(arguments.size() + 1);
  argv.push_back(const_cast<char*>("rangequery"));
  for (const char* argument : arguments) argv.push_back(const_cast<char*>(argument));
  return rq::read_command_line(static_cast<int>(argv.size()), argv.data());
}

}  // namespace

TEST(CommandLineConfiguration, DefaultsToOrderedVector) {
  const auto settings = read_arguments({});

  ASSERT_TRUE(settings.has_value());
  EXPECT_FALSE(settings->show_help);
  EXPECT_EQ(settings->structure, rq::StructureKind::ordered_vector);
}

TEST(CommandLineConfiguration, ParsesEverySupportedStructure) {
  const std::array cases{
      std::pair{"vector", rq::StructureKind::ordered_vector},
      std::pair{"set", rq::StructureKind::std_set},
      std::pair{"avl", rq::StructureKind::avl},
      std::pair{"treap", rq::StructureKind::treap},
  };

  for (const auto& [name, expected] : cases) {
    SCOPED_TRACE(name);
    const auto settings = read_arguments({"--structure", name});
    ASSERT_TRUE(settings.has_value());
    EXPECT_FALSE(settings->show_help);
    EXPECT_EQ(settings->structure, expected);
  }
}

TEST(CommandLineConfiguration, RecognizesBothHelpOptions) {
  for (const char* option : {"-h", "--help"}) {
    SCOPED_TRACE(option);
    const auto settings = read_arguments({option});
    ASSERT_TRUE(settings.has_value());
    EXPECT_TRUE(settings->show_help);
    EXPECT_EQ(settings->structure, rq::StructureKind::ordered_vector);
  }
}

TEST(CommandLineConfiguration, HelpTakesPriorityInAnyPosition) {
  const std::array cases{
      std::initializer_list<const char*>{"--help", "--unknown"},
      std::initializer_list<const char*>{"--unknown", "--help"},
      std::initializer_list<const char*>{"--structure", "binary_tree", "--help"},
      std::initializer_list<const char*>{"--structure", "treap", "-h"},
  };

  for (const auto arguments : cases) {
    const auto settings = read_arguments(arguments);
    ASSERT_TRUE(settings.has_value());
    EXPECT_TRUE(settings->show_help);
    EXPECT_EQ(settings->structure, rq::StructureKind::ordered_vector);
  }
}

TEST(CommandLineConfiguration, RejectsUnknownStructure) {
  const auto settings = read_arguments({"--structure", "binary_tree"});

  ASSERT_FALSE(settings.has_value());
  EXPECT_EQ(settings.error(), "unknown structure; expected vector, set, avl, or treap");
}

TEST(CommandLineConfiguration, RejectsMissingStructureArgument) {
  const auto settings = read_arguments({"--structure"});

  ASSERT_FALSE(settings.has_value());
  EXPECT_EQ(settings.error(), "--structure requires vector, set, avl, or treap");
}

TEST(CommandLineConfiguration, RejectsUnknownOption) {
  const auto settings = read_arguments({"--unknown"});

  ASSERT_FALSE(settings.has_value());
  EXPECT_EQ(settings.error(), "unknown command-line argument");
}

TEST(CommandLineConfiguration, StopsAtFirstError) {
  const auto settings = read_arguments({"--unknown", "--structure"});

  ASSERT_FALSE(settings.has_value());
  EXPECT_EQ(settings.error(), "unknown command-line argument");
}

TEST(CommandLineConfiguration, ConvertsEveryStructureKindToText) {
  EXPECT_EQ(rq::to_string(rq::StructureKind::ordered_vector), std::string_view{"vector"});
  EXPECT_EQ(rq::to_string(rq::StructureKind::std_set), std::string_view{"set"});
  EXPECT_EQ(rq::to_string(rq::StructureKind::avl), std::string_view{"avl"});
  EXPECT_EQ(rq::to_string(rq::StructureKind::treap), std::string_view{"treap"});
  EXPECT_EQ(rq::to_string(static_cast<rq::StructureKind>(-1)), std::string_view{"unknown"});
}
