module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module rangequery.structures.treap;

export import rangequery.structures.contracts;

namespace rq::structures {

export class Treap final {
 public:
  Treap() {}
  ~Treap() {
    Node::clear(root_);
  }
  Treap(const Treap& other) : root_(Node::clone(other.root_)), random_(other.random_) {}
  Treap& operator=(const Treap& other) {
    if (this != &other) {
      Treap replacement(other);
      swap(replacement);
    }
    return *this;
  }
  Treap(Treap&& other) noexcept {
    swap(other);
  }
  Treap& operator=(Treap&& other) noexcept {
    if (this != &other) {
      Treap replacement(std::move(other));
      swap(replacement);
    }
    return *this;
  }

  void insert(int value) {
    Node::insert(root_, value, random_, insertion_path_);
  }
  [[nodiscard]] unsigned long long count_less(int value) const {
    return Node::count_less(root_, value);
  }
  [[nodiscard]] unsigned long long count_in_open_closed_range(int left, int right) const {
    if (left >= right) return 0;
    return Node::count_less_or_equal(root_, right) - Node::count_less_or_equal(root_, left);
  }
  [[nodiscard]] std::optional<int> select(unsigned long long one_based_index) const {
    return Node::select(root_, one_based_index);
  }
  [[nodiscard]] unsigned long long size() const {
    return Node::size(root_);
  }

  void swap(Treap& other) noexcept {
    // Paths may contain addresses of root_ and must not survive an ownership transfer.
    insertion_path_.clear();
    other.insertion_path_.clear();
    root_.swap(other.root_);
    insertion_path_.swap(other.insertion_path_);
    std::swap(random_, other.random_);
  }

 private:
  struct SplitMix64 {
    std::uint64_t state{0x8e4d'3b29'1f76'a5c1ULL};
    constexpr std::uint64_t operator()() noexcept {
      std::uint64_t value = (state += 0x9e37'79b9'7f4a'7c15ULL);
      value = (value ^ (value >> 30)) * 0xbf58'476d'1ce4'e5b9ULL;
      value = (value ^ (value >> 27)) * 0x94d0'49bb'1331'11ebULL;
      return value ^ (value >> 31);
    }
  };
  struct Node {
    Node(int key_value, std::uint64_t priority_value)
        : key(key_value), priority(priority_value) {}

    int key;
    std::uint64_t priority;
    unsigned long long subtree_size{1};
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;

    static void clear(std::unique_ptr<Node>& node) noexcept;
    static std::unique_ptr<Node> clone(const std::unique_ptr<Node>& node);
    static unsigned long long size(const std::unique_ptr<Node>& node) {
      return node ? node->subtree_size : 0;
    }
    static void update(Node& node) {
      node.subtree_size = 1 + size(node.left) + size(node.right);
    }
    static void rotate_left(std::unique_ptr<Node>& node);
    static void rotate_right(std::unique_ptr<Node>& node);
    static bool insert(std::unique_ptr<Node>& node, int value, SplitMix64& random,
                       std::vector<std::unique_ptr<Node>*>& path);
    static unsigned long long count_less(const std::unique_ptr<Node>& node, int value);
    static unsigned long long count_less_or_equal(const std::unique_ptr<Node>& node,
                                                  int value);
    static std::optional<int> select(const std::unique_ptr<Node>& node,
                                     unsigned long long index);
  };
  static_assert(sizeof(SplitMix64) == sizeof(std::uint64_t));
  std::unique_ptr<Node> root_;
  std::vector<std::unique_ptr<Node>*> insertion_path_;
  SplitMix64 random_;
};

}  // namespace rq::structures
