#include "gtest/gtest.h"
#include "context.h"

TEST(ContextTest, DupTagCreation) {
  Context c;
  ASSERT_TRUE(c.new_tag("foo"));
  ASSERT_FALSE(c.new_tag("foo"));
  ASSERT_FALSE(c.new_tag("Foo"));

  auto t = c.tag_by_value("Foo");
  ASSERT_TRUE(t);
  ASSERT_EQ(t, c.tag_by_value("Foo"));
  ASSERT_EQ(t, c.tag_by_value("FOO"));
  ASSERT_EQ(t, c.tag_by_value("fOO"));
}

TEST(ContextTest, TagParenting) {
  Context c;
  auto t1 = c.new_tag("foo");
  auto t2 = c.new_tag("bar");
  ASSERT_NE(t1, t2);

  ASSERT_TRUE(t1->set_parent(t2));
  ASSERT_FALSE(t2->set_parent(t1));
}
