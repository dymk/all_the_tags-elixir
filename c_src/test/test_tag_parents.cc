#include "test_helper.h"

class TagParentTest : public ::testing::Test {
public:
  Context ctx;
  Tag *a, *b, *c, *d, *e;

  void SetUp() {
    a = ctx.new_tag("a");
    b = ctx.new_tag("b");
    c = ctx.new_tag("c");
    d = ctx.new_tag("d");
    e = ctx.new_tag("e");
  }
};


TEST_F(TagParentTest, IsSelf) {
  ASSERT_TRUE(a->is_or_ancestor_of(a));
  ASSERT_TRUE(a->is_or_descendent_of(a));

  ASSERT_FALSE(a->is_or_ancestor_of(b));
  ASSERT_FALSE(a->is_or_descendent_of(b));

  ASSERT_EQ(ctx.root_tags, SET(Tag*, {a, b, c, d, e}));
}

TEST_F(TagParentTest, SettingTagParent) {
  ASSERT_TRUE(a->set_parent(b));
  ASSERT_EQ(ctx.root_tags, SET(Tag*, {b, c, d, e}));

  ASSERT_FALSE(a->is_or_ancestor_of(b));
  ASSERT_TRUE(a->is_or_descendent_of(b));

  ASSERT_TRUE(b->is_or_ancestor_of(a));
  ASSERT_FALSE(b->is_or_descendent_of(a));
}

TEST_F(TagParentTest, MultipleBranch) {
  ASSERT_TRUE(b->set_parent(a));
  ASSERT_TRUE(c->set_parent(b));
  ASSERT_TRUE(d->set_parent(b));

  ASSERT_EQ(ctx.root_tags, SET(Tag*, {a, e}));

  // check heiarchy of "a" -> "b"
  ASSERT_TRUE(a->is_or_ancestor_of(b));
  ASSERT_FALSE(a->is_or_descendent_of(b));
  ASSERT_FALSE(b->is_or_ancestor_of(a));
  ASSERT_TRUE(b->is_or_descendent_of(a));

  // check heiarchy of "a" -> "b" -> "c"
  ASSERT_TRUE(a->is_or_ancestor_of(c));
  ASSERT_FALSE(a->is_or_descendent_of(c));
  ASSERT_FALSE(c->is_or_ancestor_of(a));
  ASSERT_TRUE(c->is_or_descendent_of(a));

  // check heiarchy of "a" -> "b" -> "d"
  ASSERT_TRUE(a->is_or_ancestor_of(d));
  ASSERT_FALSE(a->is_or_descendent_of(d));
  ASSERT_FALSE(d->is_or_ancestor_of(a));
  ASSERT_TRUE(d->is_or_descendent_of(a));

  // check heiarchy of "b" -> "d"
  //                       -> "c"
  ASSERT_FALSE(c->is_or_ancestor_of(d));
  ASSERT_FALSE(c->is_or_descendent_of(d));
  ASSERT_FALSE(d->is_or_ancestor_of(c));
  ASSERT_FALSE(d->is_or_descendent_of(c));

  // check that 'e' is in its own separate graph
  for(auto t : {a, b, c, d}) {
    ASSERT_FALSE(e->is_or_ancestor_of(t));
    ASSERT_FALSE(e->is_or_descendent_of(t));
    ASSERT_FALSE(t->is_or_ancestor_of(e));
    ASSERT_FALSE(t->is_or_descendent_of(e));
  }

  // if a parent is unset, verify it's no longer in its prior parents' tree
  c->unset_parent();
  for(auto t : {a, b}) {
    ASSERT_FALSE(t->is_or_ancestor_of(c));
    ASSERT_FALSE(t->is_or_descendent_of(c));
    ASSERT_FALSE(c->is_or_ancestor_of(t));
    ASSERT_FALSE(c->is_or_descendent_of(t));
  }
}
