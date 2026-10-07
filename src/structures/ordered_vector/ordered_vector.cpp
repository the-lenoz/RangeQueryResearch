module;

#include <algorithm>
#include <memory>
#include <vector>

module rangequery.structures.ordered_vector;

namespace rq::structures {

struct OrderedVector::Impl {
  std::vector<int> values;
};

OrderedVector::OrderedVector() : impl_(std::make_unique<Impl>()) {}
OrderedVector::~OrderedVector() = default;

void OrderedVector::insert(const int value) {
  const auto position = std::ranges::lower_bound(impl_->values, value);
  if (position == impl_->values.end() || *position != value) {
    impl_->values.insert(position, value);
  }
}

Count OrderedVector::count_less(const int value) const {
  return static_cast<Count>(std::ranges::lower_bound(impl_->values, value) -
                            impl_->values.begin());
}

Count OrderedVector::count_in_open_closed_range(const int left, const int right) const {
  if (left >= right) return 0;
  const auto first = std::ranges::upper_bound(impl_->values, left);
  const auto last = std::ranges::upper_bound(impl_->values, right);
  return static_cast<Count>(last - first);
}

Selection OrderedVector::select(const Count one_based_index) const {
  if (one_based_index == 0 || one_based_index > impl_->values.size()) return {};
  return Selection{true, impl_->values[static_cast<std::size_t>(one_based_index - 1)]};
}

Count OrderedVector::size() const { return impl_->values.size(); }

}
