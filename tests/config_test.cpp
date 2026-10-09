#include <array>
#include <gtest/gtest.h>
#include <string_view>
#include <vector>

import rangequery.config;

namespace {

rq::Settings read_arguments(std::initializer_list<const char*> arguments) {
  std::vector<char*> argv;
  argv.reserve(arguments.size() + 1);
  argv.push_back(const_cast<char*>("rangequery"));
  for (const char* argument : arguments) argv.push_back(const_cast<char*>(argument));
  return rq::read_command_line(static_cast<int>(argv.size()), argv.data());
}

}  // namespace

TEST(CommandLineConfiguration, DefaultsToOrderedVector) {
  const auto settings = read_arguments({});

  EXPECT_TRUE(settings.valid);
  EXPECT_FALSE(settings.show_help);
  EXPECT_EQ(settings.structure, rq::StructureKind::ordered_vector);
  EXPECT_EQ(settings.error_message, nullptr);
}

TEST(CommandLineConfiguration, ParsesEverySupportedStructure) {
  const std::array cases{
      std::pair{"vector", rq::StructureKind::ordered_vector},
      std::pair{"set", rq::StructureKind::std_set},
      std::pair{"avl", rq::StructureKind::avl},
      std::pair{"treap", rq::StructureKind::treap},
  };

  for (const auto [name, expected] : cases) {
    SCOPED_TRACE(name);
    const auto settings = read_arguments({"--structure", name});
    EXPECT_TRUE(settings.valid);
    EXPECT_FALSE(settings.show_help);
    EXPECT_EQ(settings.structure, expected);
    EXPECT_EQ(settings.error_message, nullptr);
  }
}

TEST(CommandLineConfiguration, RecognizesBothHelpOptions) {
  for (const char* option : {"-h", "--help"}) {
    SCOPED_TRACE(option);
    const auto settings = read_arguments({option});
    EXPECT_TRUE(settings.valid);
    EXPECT_TRUE(settings.show_help);
    EXPECT_EQ(settings.error_message, nullptr);
  }
}

TEST(CommandLineConfiguration, ParsesStructureAlongsideHelp) {
  const auto settings = read_arguments({"--structure", "treap", "--help"});

  EXPECT_TRUE(settings.valid);
  EXPECT_TRUE(settings.show_help);
  EXPECT_EQ(settings.structure, rq::StructureKind::treap);
}

TEST(CommandLineConfiguration, RejectsUnknownStructure) {
  const auto settings = read_arguments({"--structure", "binary_tree"});

  EXPECT_FALSE(settings.valid);
  ASSERT_NE(settings.error_message, nullptr);
  EXPECT_EQ(std::string_view{settings.error_message},
            "unknown structure; expected vector, set, avl, or treap");
}

TEST(CommandLineConfiguration, RejectsMissingStructureArgument) {
  const auto settings = read_arguments({"--structure"});

  EXPECT_FALSE(settings.valid);
  ASSERT_NE(settings.error_message, nullptr);
  EXPECT_EQ(std::string_view{settings.error_message},
            "--structure requires vector, set, avl, or treap");
}

TEST(CommandLineConfiguration, RejectsUnknownOption) {
  const auto settings = read_arguments({"--unknown"});

  EXPECT_FALSE(settings.valid);
  ASSERT_NE(settings.error_message, nullptr);
  EXPECT_EQ(std::string_view{settings.error_message}, "unknown command-line argument");
}

TEST(CommandLineConfiguration, ConvertsEveryStructureKindToText) {
  EXPECT_EQ(rq::to_string(rq::StructureKind::ordered_vector), std::string_view{"ordered_vector"});
  EXPECT_EQ(rq::to_string(rq::StructureKind::std_set), std::string_view{"std_set"});
  EXPECT_EQ(rq::to_string(rq::StructureKind::avl), std::string_view{"avl"});
  EXPECT_EQ(rq::to_string(rq::StructureKind::treap), std::string_view{"treap"});
}
