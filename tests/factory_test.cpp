#include <gtest/gtest.h>
#include <array>
#include <string_view>

import rangequery.structures.factory;

TEST(StructureFactory, CreatesEverySupportedAlternative) {
  const std::array kinds{
      rq::StructureKind::ordered_vector,
      rq::StructureKind::std_set,
      rq::StructureKind::avl,
      rq::StructureKind::treap,
  };

  for (std::size_t index = 0; index < kinds.size(); ++index) {
    const auto structure = rq::structures::make_structure(kinds[index]);
    ASSERT_TRUE(structure.has_value()) << "kind=" << rq::to_string(kinds[index]);
    EXPECT_EQ(structure->index(), index);
  }
}

TEST(StructureFactory, RejectsUnknownKind) {
  const auto structure = rq::structures::make_structure(static_cast<rq::StructureKind>(-1));

  ASSERT_FALSE(structure.has_value());
  EXPECT_EQ(structure.error(), std::string_view{"unknown structure kind"});
}
