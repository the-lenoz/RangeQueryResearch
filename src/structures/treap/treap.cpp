module;

#include <memory>
#include <optional>
#include <utility>
#include <vector>

module rangequery.structures.treap;

namespace rq::structures {

// Iterative clear method is required to prevent stack overflow
// because the treap structure does not guarantee logarythmic heigth
void Treap::Node::clear(std::unique_ptr<Node>& node) noexcept {
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

std::unique_ptr<Treap::Node> Treap::Node::clone(const std::unique_ptr<Node>& node) {
  if (!node) return {};
  auto copy = std::make_unique<Node>(node->key, node->priority);
  copy->subtree_size = node->subtree_size;
  std::vector<std::pair<const Node*, Node*>> pending{{node.get(), copy.get()}};

  try {
    while (!pending.empty()) {
      const auto [source, destination] = pending.back();
      pending.pop_back();

      if (source->left) {
        destination->left = std::make_unique<Node>(source->left->key, source->left->priority);
        destination->left->subtree_size = source->left->subtree_size;
        pending.emplace_back(source->left.get(), destination->left.get());
      }
      if (source->right) {
        destination->right = std::make_unique<Node>(source->right->key, source->right->priority);
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

void Treap::Node::rotate_left(std::unique_ptr<Node>& node) {
  auto pivot = std::move(node->right);
  node->right = std::move(pivot->left);
  update(*node);
  pivot->left = std::move(node);
  update(*pivot);
  node = std::move(pivot);
}

void Treap::Node::rotate_right(std::unique_ptr<Node>& node) {
  auto pivot = std::move(node->left);
  node->left = std::move(pivot->right);
  update(*node);
  pivot->right = std::move(node);
  update(*pivot);
  node = std::move(pivot);
}

bool Treap::Node::insert(std::unique_ptr<Node>& node, int value, SplitMix64& random,
                         std::vector<std::unique_ptr<Node>*>& path) {
  path.clear();
  auto* link = &node;

  while (*link) {
    if (value == (*link)->key) return false;
    path.push_back(link);
    link = value < (*link)->key ? &(*link)->left : &(*link)->right;
  }

  *link = std::make_unique<Node>(value, random());
  while (!path.empty()) {
    auto& ancestor = *path.back();
    path.pop_back();
    if (ancestor->left && ancestor->left->priority > ancestor->priority) {
      rotate_right(ancestor);
    } else if (ancestor->right && ancestor->right->priority > ancestor->priority) {
      rotate_left(ancestor);
    } else {
      update(*ancestor);
    }
  }
  return true;
}

unsigned long long Treap::Node::count_less(const std::unique_ptr<Node>& node, int value) {
  unsigned long long result = 0;
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

unsigned long long Treap::Node::count_less_or_equal(const std::unique_ptr<Node>& node, int value) {
  unsigned long long result = 0;
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

std::optional<int> Treap::Node::select(const std::unique_ptr<Node>& node, unsigned long long index) {
  if (!node || index == 0 || index > size(node)) return std::nullopt;
  unsigned long long remaining = index;
  const Node* current = node.get();
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
