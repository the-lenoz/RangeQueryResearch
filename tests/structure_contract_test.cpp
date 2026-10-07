import rangequery.config;
import rangequery.structures.avl;
import rangequery.structures.ordered_vector;
import rangequery.structures.std_set;

#include <gtest/gtest.h>

#include <limits>

static_assert(rq::structures::OrderStatisticSet<rq::structures::OrderedVector>);
static_assert(rq::structures::OrderStatisticSet<rq::structures::StdSet>);
static_assert(rq::structures::OrderStatisticSet<rq::structures::AvlTree>);

template <rq::structures::OrderStatisticSet Set>
void check_order_statistics(Set& structure) {
  EXPECT_EQ(structure.size(), 0ULL);
  EXPECT_EQ(structure.count_less(0), 0ULL);
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

  EXPECT_EQ(structure.select(1).value, -1);
  EXPECT_EQ(structure.select(2).value, 2);
  EXPECT_EQ(structure.select(3).value, 8);
  EXPECT_FALSE(structure.select(4).has_value);
}

template <rq::structures::OrderStatisticSet Set>
void check_integer_extremes(Set& structure) {
  structure.insert(std::numeric_limits<int>::min());
  structure.insert(std::numeric_limits<int>::max());
  EXPECT_EQ(structure.count_in_open_closed_range(std::numeric_limits<int>::min(),
                                                 std::numeric_limits<int>::max()), 1ULL);
  EXPECT_EQ(structure.select(1).value, std::numeric_limits<int>::min());
  EXPECT_EQ(structure.select(2).value, std::numeric_limits<int>::max());
}

TEST(OrderedVectorContract, MatchesCommonApi) {
  rq::structures::OrderedVector structure;
  check_order_statistics(structure);
}

TEST(OrderedVectorContract, HandlesIntegerExtremes) {
  rq::structures::OrderedVector structure;
  check_integer_extremes(structure);
}

TEST(StdSetContract, MatchesCommonApi) {
  rq::structures::StdSet structure;
  check_order_statistics(structure);
}

TEST(StdSetContract, HandlesIntegerExtremes) {
  rq::structures::StdSet structure;
  check_integer_extremes(structure);
}

TEST(AvlContract, MatchesCommonApi) {
  rq::structures::AvlTree structure;
  check_order_statistics(structure);
}

TEST(AvlContract, HandlesIntegerExtremes) {
  rq::structures::AvlTree structure;
  check_integer_extremes(structure);
}

TEST(CommandLineConfiguration, ChoosesRequestedStructure) {
  char program[] = "rangequery";
  char option[] = "--structure";
  char implementation[] = "avl";
  char* arguments[] = {program, option, implementation};

  const auto settings = rq::read_command_line(3, arguments);
  EXPECT_TRUE(settings.valid);
  EXPECT_EQ(settings.structure, rq::StructureKind::avl);
}
