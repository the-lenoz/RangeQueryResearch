module;

#include <iterator>
#include <memory>
#include <set>
#include <utility>

module rangequery.structures.std_set;

namespace rq::structures {

struct StdSet::Impl {
  std::set<int> values;
};

StdSet::StdSet() : impl_(std::make_unique<Impl>()) {}
StdSet::~StdSet() = default;
StdSet::StdSet(const StdSet& other) {
  if (other.impl_) impl_ = std::make_unique<Impl>(*other.impl_);
}
StdSet& StdSet::operator=(const StdSet& other) {
  if (this == &other) return *this;
  std::unique_ptr<Impl> replacement;
  if (other.impl_) replacement = std::make_unique<Impl>(*other.impl_);
  impl_ = std::move(replacement);
  return *this;
}
StdSet::StdSet(StdSet&& other) noexcept = default;
StdSet& StdSet::operator=(StdSet&& other) noexcept {
  if (this != &other) impl_ = std::move(other.impl_);
  return *this;
}

void StdSet::insert(const int value) {
  if (!impl_) impl_ = std::make_unique<Impl>();
  impl_->values.insert(value);
}

Count StdSet::count_less(const int value) const {
  if (!impl_) return 0;
  return static_cast<Count>(std::distance(impl_->values.begin(), impl_->values.lower_bound(value)));
}

Count StdSet::count_in_open_closed_range(const int left, const int right) const {
  if (!impl_ || left >= right) return 0;
  return static_cast<Count>(
      std::distance(impl_->values.upper_bound(left), impl_->values.upper_bound(right)));
}

Selection StdSet::select(const Count one_based_index) const {
  if (!impl_ || one_based_index == 0 || one_based_index > impl_->values.size()) return {};
  auto position = impl_->values.begin();
  std::advance(position, static_cast<std::ptrdiff_t>(one_based_index - 1));
  return Selection{true, *position};
}

Count StdSet::size() const {
  return impl_ ? impl_->values.size() : 0;
}

}  // namespace rq::structures
