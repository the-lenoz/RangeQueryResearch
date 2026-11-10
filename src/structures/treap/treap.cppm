module;

#include <cstdint>
#include <memory>
#include <vector>

export module rangequery.structures.treap;

export import rangequery.structures.contracts;

namespace rq::structures {

export class Treap final {
 public:
  Treap();
  ~Treap();
  Treap(const Treap& other);
  Treap& operator=(const Treap& other);
  Treap(Treap&& other) noexcept;
  Treap& operator=(Treap&& other) noexcept;

  void insert(int value);
  [[nodiscard]] Count count_less(int value) const;
  [[nodiscard]] Count count_in_open_closed_range(int left, int right) const;
  [[nodiscard]] Selection select(Count one_based_index) const;
  [[nodiscard]] Count size() const;

 private:
  struct Node;
  struct SplitMix64 {
    std::uint64_t state{0x8e4d'3b29'1f76'a5c1ULL};
    constexpr std::uint64_t operator()() noexcept;
  };
  static_assert(sizeof(SplitMix64) == sizeof(std::uint64_t));
  std::unique_ptr<Node> root_;
  std::vector<std::unique_ptr<Node>*> insertion_path_;
  SplitMix64 random_;

  void swap(Treap& other) noexcept;
};

}  // namespace rq::structures
