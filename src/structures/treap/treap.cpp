module;

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

module rangequery.structures.treap;

namespace rq::structures {

constexpr std::uint64_t Treap::SplitMix64::operator()() noexcept {
  std::uint64_t value = (state += 0x9e37'79b9'7f4a'7c15ULL);
  value = (value ^ (value >> 30)) * 0xbf58'476d'1ce4'e5b9ULL;
  value = (value ^ (value >> 27)) * 0x94d0'49bb'1331'11ebULL;
  return value ^ (value >> 31);
}

struct Treap::Node {
  Node(const int key_value, const std::uint64_t priority_value)
      : key(key_value), priority(priority_value) {}

  int key;
  std::uint64_t priority;
  Count subtree_size{1};
  std::unique_ptr<Node> left;
  std::unique_ptr<Node> right;

  static void clear(std::unique_ptr<Node>& node) noexcept;
  static std::unique_ptr<Node> clone(const std::unique_ptr<Node>& node);
  static Count size(const std::unique_ptr<Node>& node);
  static void update(Node& node);
  static void rotate_left(std::unique_ptr<Node>& node);
  static void rotate_right(std::unique_ptr<Node>& node);
  static bool insert(std::unique_ptr<Node>& node, const int value, SplitMix64& random,
                     std::vector<std::unique_ptr<Node>*>& path);
  static Count count_less(const std::unique_ptr<Node>& node, const int value);
  static Count count_less_or_equal(const std::unique_ptr<Node>& node, const int value);
  static Selection select(const std::unique_ptr<Node>& node, const Count index);
};

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

Count Treap::Node::size(const std::unique_ptr<Node>& node) {
  return node ? node->subtree_size : 0;
}

void Treap::Node::update(Node& node) {
  node.subtree_size = 1 + size(node.left) + size(node.right);
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

bool Treap::Node::insert(std::unique_ptr<Node>& node, const int value, SplitMix64& random,
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

Count Treap::Node::count_less(const std::unique_ptr<Node>& node, const int value) {
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

Count Treap::Node::count_less_or_equal(const std::unique_ptr<Node>& node, const int value) {
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

Selection Treap::Node::select(const std::unique_ptr<Node>& node, const Count index) {
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

Treap::Treap() {
  static_assert([] {
    SplitMix64 random;
    return random() == 0x3254'5d91'd39b'4e63ULL && random() == 0x86aa'ae25'8609'ca2bULL &&
           random() == 0x3fc7'3a4a'c2c7'4222ULL && random() == 0x4641'9278'3227'2e53ULL;
  }());
}
Treap::~Treap() {
  Node::clear(root_);
}
Treap::Treap(const Treap& other) : root_(Node::clone(other.root_)), random_(other.random_) {}
Treap& Treap::operator=(const Treap& other) {
  if (this != &other) {
    Treap replacement(other);
    swap(replacement);
  }
  return *this;
}
Treap::Treap(Treap&& other) noexcept : Treap() {
  swap(other);
}
Treap& Treap::operator=(Treap&& other) noexcept {
  if (this != &other) {
    Treap replacement(std::move(other));
    swap(replacement);
  }
  return *this;
}

void Treap::swap(Treap& other) noexcept {
  // Paths may contain addresses of root_ and must not survive an ownership transfer.
  insertion_path_.clear();
  other.insertion_path_.clear();
  root_.swap(other.root_);
  insertion_path_.swap(other.insertion_path_);
  std::swap(random_, other.random_);
}

void Treap::insert(const int value) {
  Node::insert(root_, value, random_, insertion_path_);
}
Count Treap::count_less(const int value) const {
  return Node::count_less(root_, value);
}
Count Treap::count_in_open_closed_range(const int left, const int right) const {
  if (left >= right) return 0;
  return Node::count_less_or_equal(root_, right) - Node::count_less_or_equal(root_, left);
}
Selection Treap::select(const Count one_based_index) const {
  return Node::select(root_, one_based_index);
}
Count Treap::size() const {
  return Node::size(root_);
}

}  // namespace rq::structures
