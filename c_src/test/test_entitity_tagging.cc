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

TEST_F(EntityAndTagTest, MatchesQuery) {
  e1->add_tag(foo);
  e2->add_tag(bar);

  auto *q_foo = build_lit(foo);
  auto *q_bar = build_lit(bar);

  auto ret = query(c, *q_foo);
  ASSERT_EQ(ret, SET(Entity*, {e1}));

  QueryClauseBin q_or(QueryClauseOr, q_foo, q_bar);
  ret = query(c, q_or);
  ASSERT_EQ(ret, SET(Entity*, {e1, e2}));
}

TEST_F(EntityAndTagTest, ParentMatches) {
  // foo's parent is bar, so either
  // "foo" or "bar" should match e1
  e1->add_tag(foo);
  foo->set_parent(bar);

  auto q_foo = build_lit(foo);
  auto q_bar = build_lit(bar);

  auto ret = query(c, *q_foo);
  ASSERT_EQ(SET(Entity*, {e1}), ret);

  ret = query(c, *q_bar);
  ASSERT_EQ(SET(Entity*, {e1}), ret);

  delete q_foo;
  delete q_bar;
}

TEST_F(EntityAndTagTest, NumEntities) {
  // foo's parent is bar, so either
  // "foo" or "bar" should match e1
  foo->set_parent(bar);
  e1->add_tag(foo);

  ASSERT_EQ(1, foo->entity_count());
  ASSERT_EQ(0, bar->entity_count());

  ASSERT_FALSE(e1->remove_tag(bar));
  ASSERT_EQ(1, foo->entity_count());
  ASSERT_EQ(0, bar->entity_count());

  ASSERT_TRUE(e1->remove_tag(foo));
  ASSERT_EQ(0, foo->entity_count());
  ASSERT_EQ(0, bar->entity_count());
}
