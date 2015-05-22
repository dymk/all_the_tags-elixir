#include "test_helper.h"

class TagImplicationTest : public ::testing::Test {
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

TEST_F(TagImplicationTest, MatchesEntity) {
  auto e = ctx.new_entity();
  e->add_tag(a);

  auto q = build_lit(b);
  auto res = query(ctx, *q);
  ASSERT_EQ(SET(Entity*, {}), res);
  delete q;

  // e tagged with 'a' implies tagged with 'b'
  a->imply(b);

  q = build_lit(b);
  q->debug_print();
  res = query(ctx, *q);
  ASSERT_EQ(SET(Entity*, {e}), res);
  delete q;
}
