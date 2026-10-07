module;

#include <experimental/propagate_const>
#include <memory>

export module rangequery.structures.avl;

export import rangequery.structures.contracts;

export namespace rq::structures {

class AvlTree final {
 public:
  AvlTree();
  ~AvlTree();
  AvlTree(const AvlTree&) = delete;
  AvlTree& operator=(const AvlTree&) = delete;

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
