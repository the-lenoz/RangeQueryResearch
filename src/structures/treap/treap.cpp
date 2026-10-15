module;

#include <cstdint>
#include <memory>
#include <random>
#include <utility>
#include <vector>

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
  std::vector<std::unique_ptr<Node>*> insertion_path;

  Impl() = default;
  Impl(const Impl& other) : root(clone(other.root)), random(other.random) {}
  ~Impl() { clear(root); }

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

  static bool insert(std::unique_ptr<Node>& node, const int value, std::mt19937_64& random,
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
    if (!node || index == 0 || index > size(node)) return {};
    Count remaining = index;
    const Node* current = node.get();
    while (current) {
      const Count left_size = size(current->left);
      if (remaining == left_size + 1) return Selection{true, current->key};
      if (remaining <= left_size) {
        current = current->left.get();
      } else {
        remaining -= left_size + 1;
        current = current->right.get();
      }
    }
    return {};
  }
};

Treap::Treap() : impl_(std::make_unique<Impl>()) {}
Treap::~Treap() = default;
Treap::Treap(const Treap& other) {
  if (other.impl_) impl_ = std::make_unique<Impl>(*other.impl_);
}
Treap& Treap::operator=(const Treap& other) {
  if (this == &other) return *this;
  std::unique_ptr<Impl> replacement;
  if (other.impl_) replacement = std::make_unique<Impl>(*other.impl_);
  impl_ = std::move(replacement);
  return *this;
}
Treap::Treap(Treap&& other) noexcept = default;
Treap& Treap::operator=(Treap&& other) noexcept {
  if (this != &other) impl_ = std::move(other.impl_);
  return *this;
}

void Treap::insert(const int value) {
  if (!impl_) impl_ = std::make_unique<Impl>();
  Impl::insert(impl_->root, value, impl_->random, impl_->insertion_path);
}
Count Treap::count_less(const int value) const {
  return impl_ ? Impl::count_less(impl_->root, value) : 0;
}
Count Treap::count_in_open_closed_range(const int left, const int right) const {
  if (!impl_ || left >= right) return 0;
  return Impl::count_less_or_equal(impl_->root, right) -
         Impl::count_less_or_equal(impl_->root, left);
}
Selection Treap::select(const Count one_based_index) const {
  return impl_ ? Impl::select(impl_->root, one_based_index) : Selection{};
}
Count Treap::size() const { return impl_ ? Impl::size(impl_->root) : 0; }

}  // namespace rq::structures
