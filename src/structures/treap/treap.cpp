module;

#include <cstdint>
#include <memory>
#include <random>

module rangequery.structures.treap;

namespace rq::structures {

struct Treap::Impl {
  struct Node {
    Node(const int key_value, const std::uint64_t priority_value)
        : key(key_value), priority(priority_value) {}

    int key;
    std::uint64_t priority;
    Count subtree_size{1};
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
  };

  static constexpr std::uint64_t kSeed = 0x8e4d'3b29'1f76'a5c1ULL;

  std::unique_ptr<Node> root;
  std::mt19937_64 random{kSeed};

  static Count size(const std::unique_ptr<Node>& node) { return node ? node->subtree_size : 0; }

  static void update(Node& node) { node.subtree_size = 1 + size(node.left) + size(node.right); }

  static void rotate_left(std::unique_ptr<Node>& node) {
    auto pivot = std::move(node->right);
    node->right = std::move(pivot->left);
    update(*node);
    pivot->left = std::move(node);
    update(*pivot);
    node = std::move(pivot);
  }

  static void rotate_right(std::unique_ptr<Node>& node) {
    auto pivot = std::move(node->left);
    node->left = std::move(pivot->right);
    update(*node);
    pivot->right = std::move(node);
    update(*pivot);
    node = std::move(pivot);
  }

  static bool insert(std::unique_ptr<Node>& node, const int value, std::mt19937_64& random) {
    if (!node) {
      node = std::make_unique<Node>(value, random());
      return true;
    }
    if (value == node->key) return false;

    const bool inserted = value < node->key ? insert(node->left, value, random)
                                             : insert(node->right, value, random);
    if (!inserted) return false;

    if (node->left && node->left->priority > node->priority) {
      rotate_right(node);
    } else if (node->right && node->right->priority > node->priority) {
      rotate_left(node);
    } else {
      update(*node);
    }
    return true;
  }

  static Count count_less(const std::unique_ptr<Node>& node, const int value) {
    if (!node) return 0;
    if (value <= node->key) return count_less(node->left, value);
    return size(node->left) + 1 + count_less(node->right, value);
  }

  static Count count_less_or_equal(const std::unique_ptr<Node>& node, const int value) {
    if (!node) return 0;
    if (value < node->key) return count_less_or_equal(node->left, value);
    return size(node->left) + 1 + count_less_or_equal(node->right, value);
  }

  static Selection select(const std::unique_ptr<Node>& node, const Count index) {
    if (!node || index == 0 || index > size(node)) return {};
    const Count left_size = size(node->left);
    if (index == left_size + 1) return Selection{true, node->key};
    if (index <= left_size) return select(node->left, index);
    return select(node->right, index - left_size - 1);
  }
};

Treap::Treap() : impl_(std::make_unique<Impl>()) {}
Treap::~Treap() = default;

void Treap::insert(const int value) { Impl::insert(impl_->root, value, impl_->random); }
Count Treap::count_less(const int value) const { return Impl::count_less(impl_->root, value); }
Count Treap::count_in_open_closed_range(const int left, const int right) const {
  if (left >= right) return 0;
  return Impl::count_less_or_equal(impl_->root, right) -
         Impl::count_less_or_equal(impl_->root, left);
}
Selection Treap::select(const Count one_based_index) const {
  return Impl::select(impl_->root, one_based_index);
}
Count Treap::size() const { return Impl::size(impl_->root); }

}  // namespace rq::structures
