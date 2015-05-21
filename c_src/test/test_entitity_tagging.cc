#include "gtest/gtest.h"
#include "context.h"
#include "test_helper.h"

TEST(EntityTest, MakeEntity) {
  Context c;
  auto entity = c.new_entity();
  ASSERT_TRUE(entity);
}

class EntityAndTagTest : public ::testing::Test {
public:
  Context c;
  Entity *e1, *e2;
  Tag *foo, *bar;

  void SetUp() {
    e1 = c.new_entity();
    e2 = c.new_entity();
    assert(e1 && e2);
    foo = c.new_tag("foo");
    bar = c.new_tag("bar");
  }
};

TEST_F(EntityAndTagTest, TagEntity) {
  ASSERT_TRUE(e1->add_tag(foo));
  ASSERT_EQ(e1->tags, SET(Tag*, {foo}));

  ASSERT_TRUE(e1->remove_tag(foo));
  ASSERT_FALSE(e1->remove_tag(foo));
  ASSERT_EQ(e1->tags, SET(Tag*, {}));
}
