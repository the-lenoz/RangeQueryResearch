module;

#include <memory>
#include <vector>
#include <optional>

export module rangequery.structures.avl;

export import rangequery.structures.contracts;

namespace rq::structures {

export class AvlTree final {
 public:
  AvlTree() = default;
  ~AvlTree() = default;
  AvlTree(const AvlTree& other) : root_(Node::clone(other.root_)) {}
  AvlTree& operator=(const AvlTree& other) {
    if (this != &other) {
      AvlTree replacement(other);
      swap(replacement);
    }
    return *this;
  }
  AvlTree(AvlTree&& other) noexcept : AvlTree() {
    swap(other);
  }
  AvlTree& operator=(AvlTree&& other) noexcept {
    if (this != &other) {
      AvlTree replacement(std::move(other));
      swap(replacement);
    }
    return *this;
  }

  void swap(AvlTree& other) noexcept {
    // Paths may contain addresses of root_ and must not survive
    // an ownership transfer.
    insertion_path_.clear();
    other.insertion_path_.clear();
    root_.swap(other.root_);
    insertion_path_.swap(other.insertion_path_);
  }

  void insert(int value) {
    if (!root_) {
      root_ = std::make_unique<Node>(value);
      return;
    }
    insertion_path_.clear();
    insertion_path_.push_back(&root_);
    root_->insert(value, insertion_path_);
  }
  [[nodiscard]] unsigned long long count_less(int value) const {
    return root_ ? root_->count_less(value) : 0;
  }
  [[nodiscard]] unsigned long long count_in_open_closed_range(int left, int right) const {
    if (left >= right || !root_) return 0;
    return root_->count_less_or_equal(right) - root_->count_less_or_equal(left);
  }
  [[nodiscard]] std::optional<int> select(unsigned long long one_based_index) const {
    return root_ ? root_->select(one_based_index) : std::nullopt;
  }
  [[nodiscard]] unsigned long long size() const {
    return Node::size(root_);
  }

 private:
  struct Node {
    explicit Node(int key_value) : key(key_value) {}
    static void rotate_left(std::unique_ptr<Node>& node);
    static void rotate_right(std::unique_ptr<Node>& node);
    static void rebalance(std::unique_ptr<Node>& node);
    void insert(int value, std::vector<std::unique_ptr<Node>*>& path);
    [[nodiscard]] unsigned long long count_less(int value) const;
    [[nodiscard]] unsigned long long count_less_or_equal(int value) const;

    int key;
    unsigned height{1};
    unsigned long long subtree_size{1};

    static std::unique_ptr<Node> clone(const std::unique_ptr<Node>& node);
    static unsigned long long size(const std::unique_ptr<Node>& node) {
      return node ? node->subtree_size : 0;
    }
    [[nodiscard]] std::optional<int> select(unsigned long long index) const;
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;

    void update() {
      height = 1 + std::max(left ? left->height : 0, right ? right->height : 0);
      subtree_size = 1 + (left ? left->subtree_size : 0) + (right ? right->subtree_size : 0);
    }
    [[nodiscard]] int balance_factor() const {
      return static_cast<int>(left ? left->height : 0) -
             static_cast<int>(right ? right->height : 0);
    }
  };

  std::unique_ptr<Node> root_;
  std::vector<std::unique_ptr<Node>*> insertion_path_;
};

}  // namespace rq::structures
