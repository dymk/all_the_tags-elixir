#include "gtest/gtest.h"
#include "context.h"
#include "test_helper.h"

TEST(ContextTest, DupTagCreation) {
  Context c;
  ASSERT_TRUE(c.new_tag("foo"));
  ASSERT_FALSE(c.new_tag("foo"));
  ASSERT_TRUE(c.new_tag("Foo"));

  ASSERT_TRUE(c.tag_by_value("Foo"));
  ASSERT_TRUE(c.tag_by_value("foo"));
  ASSERT_NE(c.tag_by_value("Foo"), c.tag_by_value("foo"));
}
