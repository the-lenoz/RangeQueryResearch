#include <algorithm>
#include <array>
#include <gtest/gtest.h>

#include <cstdint>
#include <iterator>
#include <limits>
#include <random>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

import rangequery.structures.avl;
import rangequery.structures.ordered_vector;
import rangequery.structures.std_set;
import rangequery.structures.treap;

static_assert(rq::structures::OrderStatisticSet<rq::structures::OrderedVector>);
static_assert(rq::structures::OrderStatisticSet<rq::structures::StdSet>);
static_assert(rq::structures::OrderStatisticSet<rq::structures::AvlTree>);
static_assert(rq::structures::OrderStatisticSet<rq::structures::Treap>);

template <typename Set>
constexpr bool has_expected_value_semantics =
    std::is_copy_constructible_v<Set> && std::is_copy_assignable_v<Set> &&
    std::is_nothrow_move_constructible_v<Set> && std::is_nothrow_move_assignable_v<Set>;

static_assert(has_expected_value_semantics<rq::structures::OrderedVector>);
static_assert(has_expected_value_semantics<rq::structures::StdSet>);
static_assert(has_expected_value_semantics<rq::structures::AvlTree>);
static_assert(has_expected_value_semantics<rq::structures::Treap>);

template <rq::structures::OrderStatisticSet Set>
void check_order_statistics(Set& structure) {
  EXPECT_EQ(structure.size(), 0ULL);
  EXPECT_EQ(structure.count_less(0), 0ULL);
  EXPECT_FALSE(structure.select(0).has_value);
  EXPECT_FALSE(structure.select(1).has_value);

  structure.insert(8);
  structure.insert(2);
  structure.insert(-1);
  structure.insert(2);

  EXPECT_EQ(structure.size(), 3ULL);
  EXPECT_EQ(structure.count_less(2), 1ULL);
  EXPECT_EQ(structure.count_less(3), 2ULL);
  EXPECT_EQ(structure.count_in_open_closed_range(-1, 8), 2ULL);
  EXPECT_EQ(structure.count_in_open_closed_range(8, -1), 0ULL);

  const auto first = structure.select(1);
  const auto second = structure.select(2);
  const auto third = structure.select(3);
  EXPECT_TRUE(first.has_value);
  EXPECT_TRUE(second.has_value);
  EXPECT_TRUE(third.has_value);
  EXPECT_EQ(first.value, -1);
  EXPECT_EQ(second.value, 2);
  EXPECT_EQ(third.value, 8);
  EXPECT_FALSE(structure.select(4).has_value);
}

template <rq::structures::OrderStatisticSet Set>
void check_integer_extremes(Set& structure) {
  structure.insert(std::numeric_limits<int>::min());
  structure.insert(std::numeric_limits<int>::max());
  EXPECT_EQ(structure.count_in_open_closed_range(std::numeric_limits<int>::min(),
                                                 std::numeric_limits<int>::max()),
            1ULL);
  const auto first = structure.select(1);
  const auto second = structure.select(2);
  EXPECT_TRUE(first.has_value);
  EXPECT_TRUE(second.has_value);
  EXPECT_EQ(first.value, std::numeric_limits<int>::min());
  EXPECT_EQ(second.value, std::numeric_limits<int>::max());
}

template <rq::structures::OrderStatisticSet Set>
void expect_matches_reference(const Set& structure, const std::set<int>& expected) {
  EXPECT_EQ(structure.size(), expected.size());

  for (const int value : {std::numeric_limits<int>::min(), -101, -7, -1, 0, 1, 8, 101,
                          std::numeric_limits<int>::max()}) {
    const auto expected_count = static_cast<rq::structures::Count>(
        std::distance(expected.begin(), expected.lower_bound(value)));
    EXPECT_EQ(structure.count_less(value), expected_count) << "value=" << value;
  }

  for (const auto [left, right] :
       std::array{std::pair{std::numeric_limits<int>::min(), std::numeric_limits<int>::max()},
                  std::pair{-101, 101}, std::pair{-7, 8}, std::pair{0, 0}, std::pair{8, -7}}) {
    const auto actual = structure.count_in_open_closed_range(left, right);
    if (left >= right) {
      EXPECT_EQ(actual, 0ULL) << "range=(" << left << ", " << right << ']';
      continue;
    }
    const auto expected_count = static_cast<rq::structures::Count>(
        std::distance(expected.upper_bound(left), expected.upper_bound(right)));
    EXPECT_EQ(actual, expected_count) << "range=(" << left << ", " << right << ']';
  }

  for (rq::structures::Count index = 0; index <= expected.size() + 1; ++index) {
    const auto selection = structure.select(index);
    if (index == 0 || index > expected.size()) {
      EXPECT_FALSE(selection.has_value) << "index=" << index;
      continue;
    }
    EXPECT_TRUE(selection.has_value) << "index=" << index;
    const auto iterator = std::next(expected.begin(), static_cast<long long>(index - 1));
    EXPECT_EQ(selection.value, *iterator) << "index=" << index;
  }
}

template <rq::structures::OrderStatisticSet Set>
void check_differential_insertions(Set& structure) {
  const std::array values{
      0,
      8,
      -7,
      8,
      std::numeric_limits<int>::min(),
      3,
      -1,
      101,
      std::numeric_limits<int>::max(),
      -101,
      0,
      1,
      -7,
      42,
      -42,
  };
  std::set<int> expected;

  for (const int value : values) {
    structure.insert(value);
    expected.insert(value);
    expect_matches_reference(structure, expected);
  }
}

template <rq::structures::OrderStatisticSet Set>
void check_all_small_insertion_orders() {
  std::array values{-2, -1, 0, 1};
  do {
    Set structure;
    std::set<int> expected;
    for (const int value : values) {
      structure.insert(value);
      expected.insert(value);
      expect_matches_reference(structure, expected);
    }
  } while (std::next_permutation(values.begin(), values.end()));
}

template <rq::structures::OrderStatisticSet Set>
void insert_all(Set& structure, const std::set<int>& values) {
  for (const int value : values) structure.insert(value);
}

template <typename Set>
class ValueSemanticsTest : public ::testing::Test {};

using StructureImplementations =
    ::testing::Types<rq::structures::OrderedVector, rq::structures::StdSet, rq::structures::AvlTree,
                     rq::structures::Treap>;
TYPED_TEST_SUITE(ValueSemanticsTest, StructureImplementations);

template <typename Set>
class StructureBehaviorTest : public ::testing::Test {};

TYPED_TEST_SUITE(StructureBehaviorTest, StructureImplementations);

TYPED_TEST(StructureBehaviorTest, EmptySetAnswersEveryQueryWithoutSpecialCases) {
  const TypeParam structure;

  EXPECT_EQ(structure.size(), 0ULL);
  for (const int value : {std::numeric_limits<int>::min(), -1, 0, 1,
                          std::numeric_limits<int>::max()}) {
    EXPECT_EQ(structure.count_less(value), 0ULL) << "value=" << value;
  }

  for (const auto [left, right] :
       std::array{std::pair{std::numeric_limits<int>::min(),
                            std::numeric_limits<int>::max()},
                  std::pair{-1, 1}, std::pair{0, 0}, std::pair{1, -1}}) {
    EXPECT_EQ(structure.count_in_open_closed_range(left, right), 0ULL)
        << "range=(" << left << ", " << right << ']';
  }

  EXPECT_FALSE(structure.select(0).has_value);
  EXPECT_FALSE(structure.select(1).has_value);
  EXPECT_FALSE(structure.select(std::numeric_limits<rq::structures::Count>::max()).has_value);
}

TYPED_TEST(StructureBehaviorTest, RepeatedInsertionsNeverChangeSetCardinality) {
  TypeParam structure;
  const std::array values{std::numeric_limits<int>::min(), -7, 0, 42,
                          std::numeric_limits<int>::max()};

  for (int repetition = 0; repetition < 20; ++repetition) {
    for (const int value : values) structure.insert(value);
  }

  expect_matches_reference(structure, std::set<int>{values.begin(), values.end()});
}

TYPED_TEST(StructureBehaviorTest, RangeIsOpenOnTheLeftAndClosedOnTheRight) {
  TypeParam structure;
  const std::set<int> expected{std::numeric_limits<int>::min(), -4, -1, 0, 3, 9,
                               std::numeric_limits<int>::max()};
  insert_all(structure, expected);

  const std::array endpoints{std::numeric_limits<int>::min(), -5, -4, -1, 0, 1, 3, 9, 10,
                             std::numeric_limits<int>::max()};
  for (const int left : endpoints) {
    for (const int right : endpoints) {
      const auto actual = structure.count_in_open_closed_range(left, right);
      const auto reference = left >= right
                                 ? 0ULL
                                 : static_cast<unsigned long long>(std::distance(
                                       expected.upper_bound(left), expected.upper_bound(right)));
      EXPECT_EQ(actual, reference) << "range=(" << left << ", " << right << ']';
    }
  }
}

TYPED_TEST(StructureBehaviorTest, RankCountsOnlyStrictlySmallerKeys) {
  TypeParam structure;
  const std::set<int> expected{std::numeric_limits<int>::min(), -4, -1, 0, 3, 9,
                               std::numeric_limits<int>::max()};
  insert_all(structure, expected);

  for (const int probe : {std::numeric_limits<int>::min(), -5, -4, -3, -1, 0, 1, 3, 4, 9, 10,
                          std::numeric_limits<int>::max()}) {
    const auto reference = static_cast<rq::structures::Count>(
        std::distance(expected.begin(), expected.lower_bound(probe)));
    EXPECT_EQ(structure.count_less(probe), reference) << "probe=" << probe;
  }
}

TYPED_TEST(StructureBehaviorTest, SelectUsesOneBasedIndexAcrossTheWholeSet) {
  TypeParam structure;
  std::set<int> expected;
  for (int value = -128; value <= 128; ++value) {
    structure.insert(value);
    expected.insert(value);
  }

  EXPECT_FALSE(structure.select(0).has_value);
  rq::structures::Count index = 1;
  for (const int value : expected) {
    const auto selection = structure.select(index++);
    ASSERT_TRUE(selection.has_value);
    EXPECT_EQ(selection.value, value);
  }
  EXPECT_FALSE(structure.select(index).has_value);
  EXPECT_FALSE(structure.select(std::numeric_limits<rq::structures::Count>::max()).has_value);
}

TYPED_TEST(StructureBehaviorTest, HandlesAscendingAndDescendingInsertionOrders) {
  TypeParam ascending;
  TypeParam descending;
  std::set<int> expected;

  for (int value = -512; value <= 512; ++value) {
    ascending.insert(value);
    expected.insert(value);
  }
  for (int value = 512; value >= -512; --value) descending.insert(value);

  expect_matches_reference(ascending, expected);
  expect_matches_reference(descending, expected);
  for (rq::structures::Count index = 1; index <= expected.size(); ++index) {
    const auto ascending_value = ascending.select(index);
    const auto descending_value = descending.select(index);
    ASSERT_TRUE(ascending_value.has_value);
    ASSERT_TRUE(descending_value.has_value);
    EXPECT_EQ(ascending_value.value, descending_value.value) << "index=" << index;
  }
}

TYPED_TEST(StructureBehaviorTest, MatchesStdSetForDeterministicRandomWorkload) {
  TypeParam structure;
  std::set<int> expected;
  std::mt19937 random{0x5eed1234U};
  std::uniform_int_distribution<int> values{-10'000, 10'000};

  for (int operation = 0; operation < 1'000; ++operation) {
    const int value = values(random);
    structure.insert(value);
    expected.insert(value);

    if (operation % 100 == 0) expect_matches_reference(structure, expected);
  }

  for (int query = 0; query < 1'000; ++query) {
    const int first = values(random);
    const int second = values(random);
    const int left = std::min(first, second);
    const int right = std::max(first, second);

    const auto expected_rank = static_cast<rq::structures::Count>(
        std::distance(expected.begin(), expected.lower_bound(first)));
    EXPECT_EQ(structure.count_less(first), expected_rank) << "probe=" << first;

    const auto expected_range = static_cast<rq::structures::Count>(
        std::distance(expected.upper_bound(left), expected.upper_bound(right)));
    EXPECT_EQ(structure.count_in_open_closed_range(left, right), expected_range)
        << "range=(" << left << ", " << right << ']';

    const auto index = static_cast<rq::structures::Count>(random() % (expected.size() + 2));
    const auto selection = structure.select(index);
    if (index == 0 || index > expected.size()) {
      EXPECT_FALSE(selection.has_value) << "index=" << index;
    } else {
      ASSERT_TRUE(selection.has_value) << "index=" << index;
      EXPECT_EQ(selection.value,
                *std::next(expected.begin(), static_cast<std::ptrdiff_t>(index - 1)))
          << "index=" << index;
    }
  }
}

TYPED_TEST(ValueSemanticsTest, CopyConstructorCreatesIndependentDeepCopy) {
  const std::set<int> initial{std::numeric_limits<int>::min(), -7, 0, 42,
                              std::numeric_limits<int>::max()};
  TypeParam source;
  insert_all(source, initial);

  TypeParam copy(source);
  expect_matches_reference(source, initial);
  expect_matches_reference(copy, initial);

  source.insert(101);
  copy.insert(101);
  auto common_continuation = initial;
  common_continuation.insert(101);
  expect_matches_reference(source, common_continuation);
  expect_matches_reference(copy, common_continuation);

  source.insert(8);
  copy.insert(-101);
  auto expected_source = common_continuation;
  expected_source.insert(8);
  auto expected_copy = common_continuation;
  expected_copy.insert(-101);
  expect_matches_reference(source, expected_source);
  expect_matches_reference(copy, expected_copy);
}

TYPED_TEST(ValueSemanticsTest, CopyAssignmentReplacesStateAndCreatesIndependentCopy) {
  const std::set<int> source_values{-7, 0, 42};
  TypeParam source;
  insert_all(source, source_values);
  TypeParam target;
  insert_all(target, std::set<int>{-999, 999});

  TypeParam& result = (target = source);
  EXPECT_EQ(&result, &target);
  expect_matches_reference(source, source_values);
  expect_matches_reference(target, source_values);

  source.insert(101);
  target.insert(-101);
  auto expected_source = source_values;
  expected_source.insert(101);
  auto expected_target = source_values;
  expected_target.insert(-101);
  expect_matches_reference(source, expected_source);
  expect_matches_reference(target, expected_target);
}

TYPED_TEST(ValueSemanticsTest, CopyAssignmentHandlesEmptySourceAndSelfAssignment) {
  const std::set<int> values{-7, 0, 42};
  TypeParam structure;
  insert_all(structure, values);

  const TypeParam* alias = &structure;
  TypeParam& self_result = (structure = *alias);
  EXPECT_EQ(&self_result, &structure);
  expect_matches_reference(structure, values);

  TypeParam empty;
  TypeParam& empty_result = (structure = empty);
  EXPECT_EQ(&empty_result, &structure);
  expect_matches_reference(structure, {});
  expect_matches_reference(empty, {});
}

TYPED_TEST(ValueSemanticsTest, MoveConstructorLeavesReusableEmptySource) {
  const std::set<int> values{std::numeric_limits<int>::min(), -7, 0, 42,
                             std::numeric_limits<int>::max()};
  TypeParam source;
  insert_all(source, values);

  TypeParam destination(std::move(source));
  expect_matches_reference(destination, values);
  expect_matches_reference(source, {});

  TypeParam copy_of_moved_from(source);
  expect_matches_reference(copy_of_moved_from, {});
  copy_of_moved_from.insert(-101);
  expect_matches_reference(copy_of_moved_from, {-101});

  source.insert(101);
  expect_matches_reference(source, {101});
  expect_matches_reference(destination, values);
}

TYPED_TEST(ValueSemanticsTest, MoveAssignmentHandlesExistingStateAndSelfMove) {
  const std::set<int> source_values{-7, 0, 42};
  TypeParam source;
  insert_all(source, source_values);
  TypeParam target;
  insert_all(target, std::set<int>{-999, 999});

  TypeParam& move_result = (target = std::move(source));
  EXPECT_EQ(&move_result, &target);
  expect_matches_reference(target, source_values);
  expect_matches_reference(source, {});

  TypeParam assigned_from_moved;
  assigned_from_moved.insert(101);
  assigned_from_moved = source;
  expect_matches_reference(assigned_from_moved, {});

  TypeParam* alias = &target;
  TypeParam& self_move_result = (target = std::move(*alias));
  EXPECT_EQ(&self_move_result, &target);
  expect_matches_reference(target, source_values);

  source.insert(-101);
  expect_matches_reference(source, {-101});
  expect_matches_reference(target, source_values);
}

TEST(OrderedVectorContract, MatchesCommonApi) {
  rq::structures::OrderedVector structure;
  check_order_statistics(structure);
}

TEST(OrderedVectorContract, HandlesIntegerExtremes) {
  rq::structures::OrderedVector structure;
  check_integer_extremes(structure);
}

TEST(OrderedVectorContract, MatchesReferenceAfterEveryInsertion) {
  rq::structures::OrderedVector structure;
  check_differential_insertions(structure);
}

TEST(OrderedVectorContract, MatchesReferenceForEverySmallInsertionOrder) {
  check_all_small_insertion_orders<rq::structures::OrderedVector>();
}

TEST(StdSetContract, MatchesCommonApi) {
  rq::structures::StdSet structure;
  check_order_statistics(structure);
}

TEST(StdSetContract, HandlesIntegerExtremes) {
  rq::structures::StdSet structure;
  check_integer_extremes(structure);
}

TEST(StdSetContract, MatchesReferenceAfterEveryInsertion) {
  rq::structures::StdSet structure;
  check_differential_insertions(structure);
}

TEST(StdSetContract, MatchesReferenceForEverySmallInsertionOrder) {
  check_all_small_insertion_orders<rq::structures::StdSet>();
}

TEST(AvlContract, MatchesCommonApi) {
  rq::structures::AvlTree structure;
  check_order_statistics(structure);
}

TEST(AvlContract, HandlesIntegerExtremes) {
  rq::structures::AvlTree structure;
  check_integer_extremes(structure);
}

TEST(AvlContract, MatchesReferenceAfterEveryInsertion) {
  rq::structures::AvlTree structure;
  check_differential_insertions(structure);
}

TEST(AvlContract, MatchesReferenceForEverySmallInsertionOrder) {
  check_all_small_insertion_orders<rq::structures::AvlTree>();
}

TEST(TreapContract, MatchesCommonApi) {
  rq::structures::Treap structure;
  check_order_statistics(structure);
}

TEST(TreapContract, HandlesIntegerExtremes) {
  rq::structures::Treap structure;
  check_integer_extremes(structure);
}

TEST(TreapContract, MatchesReferenceAfterEveryInsertion) {
  rq::structures::Treap structure;
  check_differential_insertions(structure);
}

TEST(TreapContract, MatchesReferenceForEverySmallInsertionOrder) {
  check_all_small_insertion_orders<rq::structures::Treap>();
}
