module;

#include <set>
#include <optional>

export module rangequery.structures.std_set;

export import rangequery.structures.contracts;

namespace rq::structures {

export class StdSet final {
 public:
  StdSet() = default;
  ~StdSet() = default;
  StdSet(const StdSet& other) = default;
  StdSet& operator=(const StdSet& other) = default;
  StdSet(StdSet&& other) = default;
  StdSet& operator=(StdSet&& other) noexcept = default;

  void insert(int value) {
    values_.insert(value);
  }
  [[nodiscard]] unsigned long long count_less(int value) const {
    return static_cast<unsigned long long>(std::distance(values_.begin(), values_.lower_bound(value)));
  }
  [[nodiscard]] unsigned long long count_in_open_closed_range(int left, int right) const {
    if (left >= right) return 0;
    return static_cast<unsigned long long>(std::distance(values_.upper_bound(left), values_.upper_bound(right)));
  }
  [[nodiscard]] std::optional<int> select(unsigned long long one_based_index) const {
    if (one_based_index == 0 || one_based_index > values_.size()) {
      return std::nullopt;
    }
    auto position = values_.begin();
    std::advance(position, static_cast<std::ptrdiff_t>(one_based_index - 1));
    return *position;
  }
  [[nodiscard]] unsigned long long size() const{
  return values_.size();
}

 private:
  std::set<int> values_;
};

}  // namespace rq::structures
