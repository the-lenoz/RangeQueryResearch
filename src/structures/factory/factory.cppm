module;

#include <expected>
#include <string_view>
#include <variant>

export module rangequery.structures.factory;

export import rangequery.structures.avl;
export import rangequery.structures.kind;
export import rangequery.structures.ordered_vector;
export import rangequery.structures.std_set;
export import rangequery.structures.treap;

namespace rq::structures {

export using SelectedStructure = std::variant<OrderedVector, StdSet, AvlTree, Treap>;

export [[nodiscard]] std::expected<SelectedStructure, std::string_view> make_structure(
    StructureKind kind);

}  // namespace rq::structures
