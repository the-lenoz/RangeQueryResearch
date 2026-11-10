module;

#include <iterator>
#include <optional>
#include <set>
#include <utility>

module rangequery.structures.std_set;

namespace rq::structures {

StdSet::StdSet() = default;
StdSet::~StdSet() = default;
StdSet::StdSet(const StdSet& other) = default;
StdSet& StdSet::operator=(const StdSet& other) {
  if (this != &other) {
    StdSet replacement(other);
    values_.swap(replacement.values_);
  }
  return *this;
}
StdSet::StdSet(StdSet&& other) noexcept {
  values_.swap(other.values_);
}
StdSet& StdSet::operator=(StdSet&& other) noexcept {
  if (this != &other) {
    StdSet replacement(std::move(other));
    values_.swap(replacement.values_);
  }
  return *this;
}

void StdSet::insert(const int value) {
  values_.insert(value);
}

Count StdSet::count_less(const int value) const {
  return static_cast<Count>(std::distance(values_.begin(), values_.lower_bound(value)));
}

Count StdSet::count_in_open_closed_range(const int left, const int right) const {
  if (left >= right) return 0;
  return static_cast<Count>(std::distance(values_.upper_bound(left), values_.upper_bound(right)));
}

Selection StdSet::select(const Count one_based_index) const {
  if (one_based_index == 0 || one_based_index > values_.size()) {
    return std::nullopt;
  }
  auto position = values_.begin();
  std::advance(position, static_cast<std::ptrdiff_t>(one_based_index - 1));
  return *position;
}

Count StdSet::size() const {
  return values_.size();
}

}  // namespace rq::structures
