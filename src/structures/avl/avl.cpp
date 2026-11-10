module;

#include <algorithm>
#include <cassert>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

module rangequery.structures.avl;

namespace rq::structures {

struct AvlTree::Node {
  explicit Node(const int key_value) : key(key_value) {}
  int key;
  unsigned height{1};
  Count subtree_size{1};

  std::unique_ptr<Node> left;
  std::unique_ptr<Node> right;

  void update() {
    height = 1 + std::max(left ? left->height : 0, right ? right->height : 0);
    subtree_size = 1 + (left ? left->subtree_size : 0) + (right ? right->subtree_size : 0);
  }

  [[nodiscard]] int balance_factor() const {
    return static_cast<int>(left ? left->height : 0) - static_cast<int>(right ? right->height : 0);
  }

  static void clear(std::unique_ptr<Node>& node) noexcept;
  static std::unique_ptr<Node> clone(const std::unique_ptr<Node>& node);
  static Count size(const std::unique_ptr<Node>& node);
  static void rotate_left(std::unique_ptr<Node>& node);
  static void rotate_right(std::unique_ptr<Node>& node);
  static void rebalance(std::unique_ptr<Node>& node);
  static void insert(std::unique_ptr<Node>& node, int value,
                     std::vector<std::unique_ptr<Node>*>& path);
  static Count count_less(const std::unique_ptr<Node>& node, const int value);
  static Count count_less_or_equal(const std::unique_ptr<Node>& node, const int value);
  static Selection select(const std::unique_ptr<Node>& node, const Count index);
};

void AvlTree::Node::clear(std::unique_ptr<Node>& node) noexcept {
  while (node) {
    if (node->left) {
      auto left = std::move(node->left);
      node->left = std::move(left->right);
      left->right = std::move(node);
      node = std::move(left);
    } else {
      node = std::move(node->right);
    }
  }
}

std::unique_ptr<AvlTree::Node> AvlTree::Node::clone(const std::unique_ptr<Node>& node) {
  if (!node) return {};
  auto copy = std::make_unique<Node>(node->key);
  copy->height = node->height;
  copy->subtree_size = node->subtree_size;
  std::vector<std::pair<const Node*, Node*>> pending{{node.get(), copy.get()}};

  try {
    while (!pending.empty()) {
      const auto [source, destination] = pending.back();
      pending.pop_back();

      if (source->left) {
        destination->left = std::make_unique<Node>(source->left->key);
        destination->left->height = source->left->height;
        destination->left->subtree_size = source->left->subtree_size;
        pending.emplace_back(source->left.get(), destination->left.get());
      }
      if (source->right) {
        destination->right = std::make_unique<Node>(source->right->key);
        destination->right->height = source->right->height;
        destination->right->subtree_size = source->right->subtree_size;
        pending.emplace_back(source->right.get(), destination->right.get());
      }
    }
  } catch (...) {
    clear(copy);
    throw;
  }
  return copy;
}

Count AvlTree::Node::size(const std::unique_ptr<Node>& node) {
  return node ? node->subtree_size : 0;
}

void AvlTree::Node::rotate_left(std::unique_ptr<Node>& node) {
  assert(node && node->right);
  auto pivot = std::move(node->right);
  node->right = std::move(pivot->left);
  node->update();
  pivot->left = std::move(node);
  pivot->update();
  node = std::move(pivot);
}

void AvlTree::Node::rotate_right(std::unique_ptr<Node>& node) {
  assert(node && node->left);
  auto pivot = std::move(node->left);
  node->left = std::move(pivot->right);
  node->update();
  pivot->right = std::move(node);
  pivot->update();
  node = std::move(pivot);
}

void AvlTree::Node::rebalance(std::unique_ptr<Node>& node) {
  node->update();
  const int factor = node->balance_factor();
  if (factor > 1) {
    if (node->left->balance_factor() < 0) rotate_left(node->left);
    rotate_right(node);
  } else if (factor < -1) {
    if (node->right->balance_factor() > 0) rotate_right(node->right);
    rotate_left(node);
  }
}

void AvlTree::Node::insert(std::unique_ptr<Node>& node, int value,
                           std::vector<std::unique_ptr<Node>*>& path) {
  path.clear();
  auto* link = &node;

  while (*link) {
    if (value == (*link)->key) return;
    path.push_back(link);
    link = value < (*link)->key ? &(*link)->left : &(*link)->right;
  }

  *link = std::make_unique<Node>(value);
  while (!path.empty()) {
    rebalance(*path.back());
    path.pop_back();
  }
}

Count AvlTree::Node::count_less(const std::unique_ptr<Node>& node, const int value) {
  Count result = 0;
  const Node* current = node.get();
  while (current) {
    if (value <= current->key) {
      current = current->left.get();
    } else {
      result += size(current->left) + 1;
      current = current->right.get();
    }
  }
  return result;
}

Count AvlTree::Node::count_less_or_equal(const std::unique_ptr<Node>& node, const int value) {
  Count result = 0;
  const Node* current = node.get();
  while (current) {
    if (value < current->key) {
      current = current->left.get();
    } else {
      result += size(current->left) + 1;
      current = current->right.get();
    }
  }
  return result;
}

Selection AvlTree::Node::select(const std::unique_ptr<Node>& node, const Count index) {
  if (!node || index == 0 || index > size(node)) return std::nullopt;
  Count remaining = index;
  const Node* current = node.get();
  while (current) {
    const Count left_size = size(current->left);
    if (remaining == left_size + 1) return current->key;
    if (remaining <= left_size) {
      current = current->left.get();
    } else {
      remaining -= left_size + 1;
      current = current->right.get();
    }
  }
  return std::nullopt;
}

AvlTree::AvlTree() = default;
AvlTree::~AvlTree() {
  Node::clear(root_);
}
AvlTree::AvlTree(const AvlTree& other) : root_(Node::clone(other.root_)) {}
AvlTree& AvlTree::operator=(const AvlTree& other) {
  if (this != &other) {
    AvlTree replacement(other);
    swap(replacement);
  }
  return *this;
}
AvlTree::AvlTree(AvlTree&& other) noexcept : AvlTree() {
  swap(other);
}
AvlTree& AvlTree::operator=(AvlTree&& other) noexcept {
  if (this != &other) {
    AvlTree replacement(std::move(other));
    swap(replacement);
  }
  return *this;
}

void AvlTree::swap(AvlTree& other) noexcept {
  // Paths may contain addresses of root_ and must not survive an ownership transfer.
  insertion_path_.clear();
  other.insertion_path_.clear();
  root_.swap(other.root_);
  insertion_path_.swap(other.insertion_path_);
}

void AvlTree::insert(const int value) {
  Node::insert(root_, value, insertion_path_);
}
Count AvlTree::count_less(const int value) const {
  return Node::count_less(root_, value);
}
Count AvlTree::count_in_open_closed_range(const int left, const int right) const {
  if (left >= right) return 0;
  return Node::count_less_or_equal(root_, right) - Node::count_less_or_equal(root_, left);
}
Selection AvlTree::select(const Count one_based_index) const {
  return Node::select(root_, one_based_index);
}
Count AvlTree::size() const {
  return Node::size(root_);
}

}  // namespace rq::structures
