module;

#include <experimental/propagate_const>
#include <memory>

export module rangequery.structures.treap;

export import rangequery.structures.contracts;

export namespace rq::structures {

class Treap final {
 public:
  Treap();
  ~Treap();
  Treap(const Treap&) = delete;
  Treap& operator=(const Treap&) = delete;

  void insert(int value);
  [[nodiscard]] Count count_less(int value) const;
  [[nodiscard]] Count count_in_open_closed_range(int left, int right) const;
  [[nodiscard]] Selection select(Count one_based_index) const;
  [[nodiscard]] Count size() const;

 private:
  struct Impl;
  std::experimental::propagate_const<std::unique_ptr<Impl>> impl_;
};

}  // namespace rq::structures
