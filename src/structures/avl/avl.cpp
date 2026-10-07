module;

#include <algorithm>
#include <memory>

module rangequery.structures.avl;

namespace rq::structures {

struct AvlTree::Impl {
  struct Node {
    explicit Node(const int key_value) : key(key_value) {}
    int key;
    unsigned height{1};
    Count subtree_size{1};
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
  };

  std::unique_ptr<Node> root;

  static unsigned height(const std::unique_ptr<Node>& node) { return node ? node->height : 0; }
  static Count size(const std::unique_ptr<Node>& node) { return node ? node->subtree_size : 0; }

  static void update(Node& node) {
    node.height = 1 + std::max(height(node.left), height(node.right));
    node.subtree_size = 1 + size(node.left) + size(node.right);
  }

  static int balance_factor(const Node& node) {
    return static_cast<int>(height(node.left)) - static_cast<int>(height(node.right));
  }

  static void rotate_left(std::unique_ptr<Node>& node) {
    auto pivot = std::move(node->right);
    node->right = std::move(pivot->left);
    update(*node);
    pivot->left = std::move(node);
    update(*pivot->left);
    update(*pivot);
    node = std::move(pivot);
  }

  static void rotate_right(std::unique_ptr<Node>& node) {
    auto pivot = std::move(node->left);
    node->left = std::move(pivot->right);
    update(*node);
    pivot->right = std::move(node);
    update(*pivot->right);
    update(*pivot);
    node = std::move(pivot);
  }

  static void rebalance(std::unique_ptr<Node>& node) {
    update(*node);
    const int factor = balance_factor(*node);
    if (factor > 1) {
      if (balance_factor(*node->left) < 0) rotate_left(node->left);
      rotate_right(node);
    } else if (factor < -1) {
      if (balance_factor(*node->right) > 0) rotate_right(node->right);
      rotate_left(node);
    }
  }

  static void insert(std::unique_ptr<Node>& node, int value) {
    if (!node) {
      node = std::make_unique<Node>(value);
      return;
    }
    if (value < node->key) {
      insert(node->left, value);
    } else if (value > node->key) {
      insert(node->right, value);
    } else {
      return;
    }
    rebalance(node);
  }

  static Count count_less(const std::unique_ptr<Node>& node, int value) {
    if (!node) return 0;
    if (value <= node->key) return count_less(node->left, value);
    return size(node->left) + 1 + count_less(node->right, value);
  }

  static Count count_less_or_equal(const std::unique_ptr<Node>& node, int value) {
    if (!node) return 0;
    if (value < node->key) return count_less_or_equal(node->left, value);
    return size(node->left) + 1 + count_less_or_equal(node->right, value);
  }

  static Selection select(const std::unique_ptr<Node>& node, Count index) {
    if (!node || index == 0 || index > size(node)) return {};
    const Count left_size = size(node->left);
    if (index == left_size + 1) return Selection{true, node->key};
    if (index <= left_size) return select(node->left, index);
    return select(node->right, index - left_size - 1);
  }
};

AvlTree::AvlTree() : impl_(std::make_unique<Impl>()) {}
AvlTree::~AvlTree() = default;

void AvlTree::insert(const int value) { Impl::insert(impl_->root, value); }
Count AvlTree::count_less(int value) const { return Impl::count_less(impl_->root, value); }
Count AvlTree::count_in_open_closed_range(const int left, const int right) const {
  if (left >= right) return 0;
  return Impl::count_less_or_equal(impl_->root, right) -
         Impl::count_less_or_equal(impl_->root, left);
}
Selection AvlTree::select(Count one_based_index) const {
  return Impl::select(impl_->root, one_based_index);
}
Count AvlTree::size() const { return Impl::size(impl_->root); }

}  // namespace rq::structures
