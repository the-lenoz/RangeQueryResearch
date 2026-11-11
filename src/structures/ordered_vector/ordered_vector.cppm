module;

#include <optional>
#include <vector>

#include <algorithm>
export module rangequery.structures.ordered_vector;

export import rangequery.structures.contracts;

namespace rq::structures {

export class OrderedVector final {
 public:
  void insert(int value) {
    const auto position = std::ranges::lower_bound(values_, value);
    if (position == values_.end() || *position != value) {
      values_.insert(position, value);
    }
  }
  [[nodiscard]] unsigned long long count_less(int value) const {
    return static_cast<unsigned long long>(std::ranges::lower_bound(values_, value) - values_.begin());
  }
  [[nodiscard]] unsigned long long count_in_open_closed_range(int left, int right) const {
    if (left >= right) return 0;
    const auto first = std::ranges::upper_bound(values_, left);
    const auto last = std::ranges::upper_bound(values_, right);
    return static_cast<unsigned long long>(last - first);
  }
  [[nodiscard]] std::optional<int> select(unsigned long long one_based_index) const {
    if (one_based_index == 0 || one_based_index > values_.size()) {
      return std::nullopt;
    }
    return values_[static_cast<std::size_t>(one_based_index - 1)];
  }
  [[nodiscard]] unsigned long long size() const {
    return values_.size();
  }

 private:
  std::vector<int> values_;
};

}  // namespace rq::structures
