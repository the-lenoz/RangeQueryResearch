module;

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

module rangequery.structures.ordered_vector;

namespace rq::structures {

struct OrderedVector::Impl {
  std::vector<int> values;
};

OrderedVector::OrderedVector() : impl_(std::make_unique<Impl>()) {}
OrderedVector::~OrderedVector() = default;
OrderedVector::OrderedVector(const OrderedVector& other) {
  if (other.impl_) impl_ = std::make_unique<Impl>(*other.impl_);
}
OrderedVector& OrderedVector::operator=(const OrderedVector& other) {
  if (this == &other) return *this;
  std::unique_ptr<Impl> replacement;
  if (other.impl_) replacement = std::make_unique<Impl>(*other.impl_);
  impl_ = std::move(replacement);
  return *this;
}
OrderedVector::OrderedVector(OrderedVector&& other) noexcept = default;
OrderedVector& OrderedVector::operator=(OrderedVector&& other) noexcept {
  if (this != &other) impl_ = std::move(other.impl_);
  return *this;
}

void OrderedVector::insert(const int value) {
  if (!impl_) impl_ = std::make_unique<Impl>();
  const auto position = std::ranges::lower_bound(impl_->values, value);
  if (position == impl_->values.end() || *position != value) {
    impl_->values.insert(position, value);
  }
}

Count OrderedVector::count_less(const int value) const {
  if (!impl_) return 0;
  return static_cast<Count>(std::ranges::lower_bound(impl_->values, value) - impl_->values.begin());
}

Count OrderedVector::count_in_open_closed_range(const int left, const int right) const {
  if (!impl_ || left >= right) return 0;
  const auto first = std::ranges::upper_bound(impl_->values, left);
  const auto last = std::ranges::upper_bound(impl_->values, right);
  return static_cast<Count>(last - first);
}

Selection OrderedVector::select(const Count one_based_index) const {
  if (!impl_ || one_based_index == 0 || one_based_index > impl_->values.size()) return {};
  return Selection{true, impl_->values[static_cast<std::size_t>(one_based_index - 1)]};
}

Count OrderedVector::size() const { return impl_ ? impl_->values.size() : 0; }

}  // namespace rq::structures
