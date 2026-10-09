#include <array>
#include <gtest/gtest.h>

#include <iterator>
#include <limits>
#include <set>

import rangequery.structures.avl;
import rangequery.structures.ordered_vector;
import rangequery.structures.std_set;
import rangequery.structures.treap;

static_assert(rq::structures::OrderStatisticSet<rq::structures::OrderedVector>);
static_assert(rq::structures::OrderStatisticSet<rq::structures::StdSet>);
static_assert(rq::structures::OrderStatisticSet<rq::structures::AvlTree>);
static_assert(rq::structures::OrderStatisticSet<rq::structures::Treap>);

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
                                                 std::numeric_limits<int>::max()), 1ULL);
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

  for (const auto [left, right] : std::array{
           std::pair{std::numeric_limits<int>::min(), std::numeric_limits<int>::max()},
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
      0, 8, -7, 8, std::numeric_limits<int>::min(), 3, -1, 101,
      std::numeric_limits<int>::max(), -101, 0, 1, -7, 42, -42,
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
