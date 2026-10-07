module;

#include <iterator>
#include <memory>
#include <set>

module rangequery.structures.std_set;

namespace rq::structures {

struct StdSet::Impl {
  std::set<int> values;
};

StdSet::StdSet() : impl_(std::make_unique<Impl>()) {}
StdSet::~StdSet() = default;

void StdSet::insert(const int value) { impl_->values.insert(value); }

Count StdSet::count_less(const int value) const {
  return static_cast<Count>(std::distance(impl_->values.begin(), impl_->values.lower_bound(value)));
}

Count StdSet::count_in_open_closed_range(const int left, const int right) const {
  if (left >= right) return 0;
  return static_cast<Count>(std::distance(impl_->values.upper_bound(left),
                                           impl_->values.upper_bound(right)));
}

Selection StdSet::select(const Count one_based_index) const {
  if (one_based_index == 0 || one_based_index > impl_->values.size()) return {};
  auto position = impl_->values.begin();
  std::advance(position, static_cast<std::ptrdiff_t>(one_based_index - 1));
  return Selection{true, *position};
}

Count StdSet::size() const { return impl_->values.size(); }

}
