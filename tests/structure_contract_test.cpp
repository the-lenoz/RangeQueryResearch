#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <array>
#include <limits>
#include <optional>

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
class StructureBehaviorTest : public ::testing::Test {
  static_assert(rq::structures::OrderStatisticSet<Set>);
};
TYPED_TEST_SUITE(StructureBehaviorTest, StructureImplementations);

TYPED_TEST(StructureBehaviorTest, EmptySetAnswersQueries) {
  const TypeParam structure;

  const auto size = structure.size();
  const auto rank = structure.count_less(0);
  const auto range = structure.count_in_open_closed_range(-1, 1);
  const auto selected = structure.select(1);

  EXPECT_EQ(size, 0U);
  EXPECT_EQ(rank, 0U);
  EXPECT_EQ(range, 0U);
  EXPECT_EQ(selected, std::nullopt);
}

TYPED_TEST(StructureBehaviorTest, InsertionsProduceSortedUniqueKeys) {
  TypeParam structure;

  structure.insert(9);
  structure.insert(-4);
  structure.insert(0);
  structure.insert(0);
  const auto size = structure.size();
  const auto selected = std::array{structure.select(1), structure.select(2), structure.select(3)};

  EXPECT_EQ(size, 3U);
  EXPECT_THAT(selected, ::testing::ElementsAre(Optional(-4), Optional(0), Optional(9)));
}

TYPED_TEST(StructureBehaviorTest, RankCountsOnlyStrictlySmallerKeys) {
  TypeParam structure;
  structure.insert(-4);
  structure.insert(0);
  structure.insert(9);

  const auto at_key = structure.count_less(0);
  const auto between_keys = structure.count_less(1);

  EXPECT_EQ(at_key, 1U);
  EXPECT_EQ(between_keys, 2U);
}

TYPED_TEST(StructureBehaviorTest, RangeExcludesLeftAndIncludesRightEndpoint) {
  TypeParam structure;
  structure.insert(-4);
  structure.insert(0);
  structure.insert(9);

  const auto excludes_left = structure.count_in_open_closed_range(-4, 8);
  const auto includes_right = structure.count_in_open_closed_range(-5, 0);

  EXPECT_EQ(excludes_left, 1U);
  EXPECT_EQ(includes_right, 2U);
}

TYPED_TEST(StructureBehaviorTest, EmptyRangesReturnZero) {
  TypeParam structure;
  structure.insert(0);

  const auto no_keys = structure.count_in_open_closed_range(1, 2);
  const auto equal = structure.count_in_open_closed_range(0, 0);
  const auto reversed = structure.count_in_open_closed_range(1, -1);

  EXPECT_EQ(no_keys, 0U);
  EXPECT_EQ(equal, 0U);
  EXPECT_EQ(reversed, 0U);
}

TYPED_TEST(StructureBehaviorTest, InvalidSelectionReturnsNoValue) {
  TypeParam structure;
  structure.insert(9);

  const auto zero = structure.select(0);
  const auto past_end = structure.select(2);
  const auto largest_index = structure.select(std::numeric_limits<unsigned long long>::max());

  EXPECT_EQ(zero, std::nullopt);
  EXPECT_EQ(past_end, std::nullopt);
  EXPECT_EQ(largest_index, std::nullopt);
}

TYPED_TEST(StructureBehaviorTest, IntegerExtremesDoNotOverflowQueries) {
  TypeParam structure;
  structure.insert(highest);
  structure.insert(lowest);

  const auto selected = std::array{structure.select(1), structure.select(2)};
  const auto lowest_rank = structure.count_less(lowest);
  const auto highest_rank = structure.count_less(highest);
  const auto range = structure.count_in_open_closed_range(lowest, highest);

  EXPECT_THAT(selected, ::testing::ElementsAre(Optional(lowest), Optional(highest)));
  EXPECT_EQ(lowest_rank, 0U);
  EXPECT_EQ(highest_rank, 1U);
  EXPECT_EQ(range, 1U);
}

// These four cases exercise AVL's own balancing branches, rather than every
// insertion permutation for every implementation.
class AvlRotationTest : public ::testing::TestWithParam<std::array<int, 3>> {};

TEST_P(AvlRotationTest, BalancingPreservesOrderStatistics) {
  const auto keys = GetParam();
  rq::structures::AvlTree structure;
  structure.insert(keys[0]);
  structure.insert(keys[1]);

  structure.insert(keys[2]);
  const auto selected = std::array{structure.select(1), structure.select(2), structure.select(3)};
  const auto rank = structure.count_less(9);

  EXPECT_THAT(selected, ::testing::ElementsAre(Optional(-4), Optional(0), Optional(9)));
  EXPECT_EQ(rank, 2U);
}

INSTANTIATE_TEST_SUITE_P(Rotations, AvlRotationTest,
                         ::testing::Values(std::array{-4, 0, 9}, std::array{9, 0, -4},
                                           std::array{9, -4, 0}, std::array{-4, 9, 0}));
}  // namespace
