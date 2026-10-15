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
  AvlTree(const AvlTree& other);
  AvlTree& operator=(const AvlTree& other);
  AvlTree(AvlTree&& other) noexcept;
  AvlTree& operator=(AvlTree&& other) noexcept;

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
