export module rangequery.structures.contracts;

export namespace rq::structures {

using Count = unsigned long long;

struct Selection {
  bool has_value{false};
  int value{0};
};

template <typename Set>
concept OrderStatisticSet = requires(Set& mutable_set, const Set& const_set, int value,
                                     int left, int right, Count index) {
  mutable_set.insert(value);
  const_set.count_less(value);
  const_set.count_in_open_closed_range(left, right);
  const_set.select(index);
  const_set.size();
};

}  // namespace rq::structures
