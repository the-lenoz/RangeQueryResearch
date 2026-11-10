module;

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

module rangequery.structures.ordered_vector;

namespace rq::structures {

OrderedVector::OrderedVector() = default;
OrderedVector::~OrderedVector() = default;
OrderedVector::OrderedVector(const OrderedVector& other) = default;
OrderedVector& OrderedVector::operator=(const OrderedVector& other) {
  if (this != &other) {
    OrderedVector replacement(other);
    values_.swap(replacement.values_);
  }
  return *this;
}
OrderedVector::OrderedVector(OrderedVector&& other) noexcept {
  values_.swap(other.values_);
}
OrderedVector& OrderedVector::operator=(OrderedVector&& other) noexcept {
  if (this != &other) {
    OrderedVector replacement(std::move(other));
    values_.swap(replacement.values_);
  }
  return *this;
}

void OrderedVector::insert(const int value) {
  const auto position = std::ranges::lower_bound(values_, value);
  if (position == values_.end() || *position != value) {
    values_.insert(position, value);
  }
}

Count OrderedVector::count_less(const int value) const {
  return static_cast<Count>(std::ranges::lower_bound(values_, value) - values_.begin());
}

Count OrderedVector::count_in_open_closed_range(const int left, const int right) const {
  if (left >= right) return 0;
  const auto first = std::ranges::upper_bound(values_, left);
  const auto last = std::ranges::upper_bound(values_, right);
  return static_cast<Count>(last - first);
}

Selection OrderedVector::select(const Count one_based_index) const {
  if (one_based_index == 0 || one_based_index > values_.size()) {
    return std::nullopt;
  }
  return values_[static_cast<std::size_t>(one_based_index - 1)];
}

Count OrderedVector::size() const {
  return values_.size();
}

}  // namespace rq::structures
