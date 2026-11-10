module;

#include <memory>
#include <vector>

export module rangequery.structures.avl;

export import rangequery.structures.contracts;

namespace rq::structures {

export class AvlTree final {
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
  struct Node;
  std::unique_ptr<Node> root_;
  std::vector<std::unique_ptr<Node>*> insertion_path_;

  void swap(AvlTree& other) noexcept;
};

}  // namespace rq::structures
