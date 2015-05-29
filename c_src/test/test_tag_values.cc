#include "gtest/gtest.h"
#include "context.h"
#include "test_helper.h"

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
