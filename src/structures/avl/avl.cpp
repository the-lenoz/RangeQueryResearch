module;

#include <algorithm>
#include <cassert>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

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

    void update() {
      height = 1 + std::max(left ? left->height : 0, right ? right->height : 0);
      subtree_size = 1 + (left ? left->subtree_size : 0) + (right ? right->subtree_size : 0);
    }

    [[nodiscard]] int balance_factor() const {
      return static_cast<int>(left ? left->height : 0) -
             static_cast<int>(right ? right->height : 0);
    }
  };

  std::unique_ptr<Node> root;
  std::vector<std::unique_ptr<Node>*> insertion_path;

  Impl() = default;
  Impl(const Impl& other) : root(clone(other.root)) {}
  ~Impl() {
    clear(root);
  }

  static void clear(std::unique_ptr<Node>& node) noexcept {
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

  static std::unique_ptr<Node> clone(const std::unique_ptr<Node>& node) {
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

  static Count size(const std::unique_ptr<Node>& node) {
    return node ? node->subtree_size : 0;
  }

  static void rotate_left(std::unique_ptr<Node>& node) {
    assert(node && node->right);
    auto pivot = std::move(node->right);
    node->right = std::move(pivot->left);
    node->update();
    pivot->left = std::move(node);
    pivot->update();
    node = std::move(pivot);
  }

  static void rotate_right(std::unique_ptr<Node>& node) {
    assert(node && node->left);
    auto pivot = std::move(node->left);
    node->left = std::move(pivot->right);
    node->update();
    pivot->right = std::move(node);
    pivot->update();
    node = std::move(pivot);
  }

  static void rebalance(std::unique_ptr<Node>& node) {
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

  static void insert(std::unique_ptr<Node>& node, int value,
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

  static Count count_less(const std::unique_ptr<Node>& node, const int value) {
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

  static Count count_less_or_equal(const std::unique_ptr<Node>& node, const int value) {
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

  static Selection select(const std::unique_ptr<Node>& node, const Count index) {
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
};

AvlTree::AvlTree() : impl_(std::make_unique<Impl>()) {}
AvlTree::~AvlTree() = default;
AvlTree::AvlTree(const AvlTree& other) {
  if (other.impl_) impl_ = std::make_unique<Impl>(*other.impl_);
}
AvlTree& AvlTree::operator=(const AvlTree& other) {
  if (this == &other) return *this;
  std::unique_ptr<Impl> replacement;
  if (other.impl_) replacement = std::make_unique<Impl>(*other.impl_);
  impl_ = std::move(replacement);
  return *this;
}
AvlTree::AvlTree(AvlTree&& other) noexcept = default;
AvlTree& AvlTree::operator=(AvlTree&& other) noexcept {
  if (this != &other) impl_ = std::move(other.impl_);
  return *this;
}

void AvlTree::insert(const int value) {
  if (!impl_) impl_ = std::make_unique<Impl>();
  Impl::insert(impl_->root, value, impl_->insertion_path);
}
Count AvlTree::count_less(const int value) const {
  return impl_ ? Impl::count_less(impl_->root, value) : 0;
}
Count AvlTree::count_in_open_closed_range(const int left, const int right) const {
  if (!impl_ || left >= right) return 0;
  return Impl::count_less_or_equal(impl_->root, right) -
         Impl::count_less_or_equal(impl_->root, left);
}
Selection AvlTree::select(const Count one_based_index) const {
  return impl_ ? Impl::select(impl_->root, one_based_index) : std::nullopt;
}
Count AvlTree::size() const {
  return impl_ ? Impl::size(impl_->root) : 0;
}

}  // namespace rq::structures
