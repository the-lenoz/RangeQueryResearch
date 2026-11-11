module;

#include <cassert>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

module rangequery.structures.avl;

namespace rq::structures {

std::unique_ptr<AvlTree::Node> AvlTree::Node::clone(const std::unique_ptr<Node>& node) {
  if (!node) return {};
  auto copy = std::make_unique<Node>(node->key);
  copy->height = node->height;
  copy->subtree_size = node->subtree_size;
  std::vector<std::pair<const Node*, Node*>> pending{{node.get(), copy.get()}};

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
  return copy;
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

void AvlTree::Node::insert(int value, std::vector<std::unique_ptr<Node>*>& path) {
  Node* current = this;
  while (true) {
    if (value == current->key) return;
    auto* child = value < current->key ? &current->left : &current->right;
    if (!*child) {
      *child = std::make_unique<Node>(value);
      break;
    }
    path.push_back(child);
    current = child->get();
  }

  while (!path.empty()) {
    rebalance(*path.back());
    path.pop_back();
  }
}

unsigned long long AvlTree::Node::count_less(int value) const {
  unsigned long long result = 0;
  const Node* current = this;
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

unsigned long long AvlTree::Node::count_less_or_equal(int value) const {
  unsigned long long result = 0;
  const Node* current = this;
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

std::optional<int> AvlTree::Node::select(unsigned long long index) const {
  if (index == 0 || index > subtree_size) return std::nullopt;
  unsigned long long remaining = index;
  const Node* current = this;
  while (current) {
    const unsigned long long left_size = size(current->left);
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
}  // namespace rq::structures
