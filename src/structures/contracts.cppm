module;

#include <concepts>
#include <optional>

export module rangequery.structures.contracts;

namespace rq::structures {

export template <typename Set>
concept OrderStatisticSet =
    std::copyable<Set> &&
    requires(Set& mutable_set, const Set& const_set, int value, int left, int right, unsigned long long index) {
      mutable_set.insert(value);
      const_set.count_less(value);
      const_set.count_in_open_closed_range(left, right);
      { const_set.select(index) } -> std::same_as<std::optional<int>>;
      const_set.size();
    };

}  // namespace rq::structures
