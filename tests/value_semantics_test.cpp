#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>

import rangequery.structures.avl;
import rangequery.structures.ordered_vector;
import rangequery.structures.std_set;
import rangequery.structures.treap;

namespace {
using ::testing::Optional;
constexpr int lowest = std::numeric_limits<int>::min();
constexpr int highest = std::numeric_limits<int>::max();

using StructureImplementations =
    ::testing::Types<rq::structures::OrderedVector, rq::structures::StdSet, rq::structures::AvlTree,
                     rq::structures::Treap>;

template <typename Set>
class ValueSemanticsTest : public ::testing::Test {
  static_assert(std::is_copy_constructible_v<Set>);
  static_assert(std::is_copy_assignable_v<Set>);
  static_assert(std::is_nothrow_move_constructible_v<Set>);
  static_assert(std::is_nothrow_move_assignable_v<Set>);
};
TYPED_TEST_SUITE(ValueSemanticsTest, StructureImplementations);

TYPED_TEST(ValueSemanticsTest, CopyConstructorCreatesIndependentState) {
  TypeParam source;
  source.insert(-4);
  source.insert(9);

  TypeParam target(source);
  source.insert(0);
  target.insert(5);
  const auto source_middle = source.select(2);
  const auto target_middle = target.select(2);

  EXPECT_THAT(source_middle, Optional(0));
  EXPECT_THAT(target_middle, Optional(5));
}

TYPED_TEST(ValueSemanticsTest, CopyAssignmentCreatesIndependentState) {
  TypeParam source;
  source.insert(-4);
  source.insert(9);
  TypeParam target;
  target.insert(100);

  target = source;
  source.insert(0);
  target.insert(5);
  const auto source_middle = source.select(2);
  const auto target_middle = target.select(2);
  const auto target_size = target.size();

  EXPECT_THAT(source_middle, Optional(0));
  EXPECT_THAT(target_middle, Optional(5));
  EXPECT_EQ(target_size, 3U);
}

TYPED_TEST(ValueSemanticsTest, CopyAssignmentFromEmptyClearsTarget) {
  const TypeParam source;
  TypeParam target;
  target.insert(9);

  target = source;
  const auto size = target.size();

  EXPECT_EQ(size, 0U);
}

TYPED_TEST(ValueSemanticsTest, SelfCopyPreservesContents) {
  TypeParam source;
  source.insert(-4);
  source.insert(9);
  TypeParam& target = source;
  const auto* alias = &source;

  auto* result = &(target = *alias);
  const auto size = target.size();
  const auto first = target.select(1);
  const auto last = target.select(2);

  EXPECT_EQ(size, 2U);
  EXPECT_THAT(first, Optional(-4));
  EXPECT_THAT(last, Optional(9));
  EXPECT_EQ(result, &target);
}

TYPED_TEST(ValueSemanticsTest, MoveConstructorEmptiesSource) {
  TypeParam source;
  source.insert(-4);
  source.insert(9);

  TypeParam target(std::move(source));
  const auto destination_key = target.select(2);
  const auto size = source.size();
  const auto rank = source.count_less(0);
  const auto range = source.count_in_open_closed_range(lowest, highest);
  const auto selected = source.select(1);

  EXPECT_THAT(destination_key, Optional(9));
  EXPECT_EQ(size, 0U);
  EXPECT_EQ(rank, 0U);
  EXPECT_EQ(range, 0U);
  EXPECT_EQ(selected, std::nullopt);
}

TYPED_TEST(ValueSemanticsTest, MoveConstructorAllowsSourceReuse) {
  TypeParam source;
  source.insert(-4);
  source.insert(9);
  TypeParam target(std::move(source));

  source.insert(5);
  const auto source_size = source.size();
  const auto selected = source.select(1);
  const auto target_size = target.size();

  EXPECT_EQ(source_size, 1U);
  EXPECT_THAT(selected, Optional(5));
  EXPECT_EQ(target_size, 2U);
}

TYPED_TEST(ValueSemanticsTest, MoveAssignmentTransfersContents) {
  TypeParam source;
  source.insert(-4);
  source.insert(9);
  TypeParam target;
  target.insert(100);

  auto* result = &(target = std::move(source));
  const auto size = target.size();
  const auto first = target.select(1);
  const auto last = target.select(2);
  const auto source_size = source.size();

  EXPECT_EQ(size, 2U);
  EXPECT_THAT(first, Optional(-4));
  EXPECT_THAT(last, Optional(9));
  EXPECT_EQ(result, &target);
  EXPECT_EQ(source_size, 0U);
}

TYPED_TEST(ValueSemanticsTest, MoveAssignmentAllowsSourceReuse) {
  TypeParam source;
  source.insert(-4);
  source.insert(9);
  TypeParam target;
  target.insert(100);
  target = std::move(source);

  source.insert(5);
  const auto source_size = source.size();
  const auto selected = source.select(1);
  const auto target_size = target.size();

  EXPECT_EQ(source_size, 1U);
  EXPECT_THAT(selected, Optional(5));
  EXPECT_EQ(target_size, 2U);
}

}  // namespace
