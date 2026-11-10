module;

#include <set>

export module rangequery.structures.std_set;

export import rangequery.structures.contracts;

namespace rq::structures {

export class StdSet final {
 public:
  StdSet();
  ~StdSet();
  StdSet(const StdSet& other);
  StdSet& operator=(const StdSet& other);
  StdSet(StdSet&& other) noexcept;
  StdSet& operator=(StdSet&& other) noexcept;

  void insert(int value);
  [[nodiscard]] Count count_less(int value) const;
  [[nodiscard]] Count count_in_open_closed_range(int left, int right) const;
  [[nodiscard]] Selection select(Count one_based_index) const;
  [[nodiscard]] Count size() const;

 private:
  std::set<int> values_;
};

}  // namespace rq::structures
