#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <optional>
#include <string_view>
#include <variant>

import rangequery.structures.factory;

using namespace ::testing;

TEST(StructureFactory, RejectsUnknownKind) {
  const auto kind = static_cast<rq::StructureKind>(-1);

  const auto structure = rq::structures::make_structure(kind);

  ASSERT_FALSE(structure.has_value());
  EXPECT_EQ(structure.error(), std::string_view{"unknown structure kind"});
}

TEST(StructureFactory, CreatesEmptyVector) {
  const auto kind = rq::StructureKind::ordered_vector;

  const auto structure = rq::structures::make_structure(kind);
  const auto* selected =
      structure ? std::get_if<rq::structures::OrderedVector>(&*structure) : nullptr;
  const auto size = selected ? std::optional{selected->size()} : std::nullopt;

  ASSERT_NE(selected, nullptr);
  EXPECT_THAT(size, Optional(0U));
}

TEST(StructureFactory, CreatesEmptySet) {
  const auto kind = rq::StructureKind::std_set;

  const auto structure = rq::structures::make_structure(kind);
  const auto* selected = structure ? std::get_if<rq::structures::StdSet>(&*structure) : nullptr;
  const auto size = selected ? std::optional{selected->size()} : std::nullopt;

  ASSERT_NE(selected, nullptr);
  EXPECT_THAT(size, Optional(0U));
}

TEST(StructureFactory, CreatesEmptyAvl) {
  const auto kind = rq::StructureKind::avl;

  const auto structure = rq::structures::make_structure(kind);
  const auto* selected = structure ? std::get_if<rq::structures::AvlTree>(&*structure) : nullptr;
  const auto size = selected ? std::optional{selected->size()} : std::nullopt;

  ASSERT_NE(selected, nullptr);
  EXPECT_THAT(size, Optional(0U));
}

TEST(StructureFactory, CreatesEmptyTreap) {
  const auto kind = rq::StructureKind::treap;

  const auto structure = rq::structures::make_structure(kind);
  const auto* selected = structure ? std::get_if<rq::structures::Treap>(&*structure) : nullptr;
  const auto size = selected ? std::optional{selected->size()} : std::nullopt;

  ASSERT_NE(selected, nullptr);
  EXPECT_THAT(size, Optional(0U));
}
