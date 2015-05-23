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

TEST_F(TagImplicationTest, MakesRightDAG) {
  // initially, no SCCs (by default, nodes don't create an SCC)
  ASSERT_EQ(0, ctx.meta_nodes.size());

  // should have SCC of set {a} and of set {b}
  a->imply(b);
  ASSERT_EQ(2, ctx.meta_nodes.size());
  ASSERT_EQ(a->meta_node->parents,  SET(SCCMetaNode*, {}));
  ASSERT_EQ(a->meta_node->children, SET(SCCMetaNode*, {b->meta_node}));
  ASSERT_EQ(b->meta_node->parents,  SET(SCCMetaNode*, {a->meta_node}));
  ASSERT_EQ(b->meta_node->children, SET(SCCMetaNode*, {}));

  // should have SCC of set {a}, {b}, {c}
  b->imply(c);
  ASSERT_EQ(3, ctx.meta_nodes.size());
  ASSERT_EQ(b->meta_node->parents,  SET(SCCMetaNode*, {a->meta_node}));
  ASSERT_EQ(b->meta_node->children, SET(SCCMetaNode*, {c->meta_node}));
  ASSERT_EQ(c->meta_node->parents,  SET(SCCMetaNode*, {b->meta_node}));
  ASSERT_EQ(c->meta_node->children, SET(SCCMetaNode*, {}));

  // collapse all the SCCs into {a,c,c}
  c->imply(a);
  ASSERT_EQ(1, ctx.meta_nodes.size());

  auto mn = *(ctx.meta_nodes.begin());
  ASSERT_TRUE(mn);

  ASSERT_EQ(mn->tags,     SET(Tag*,         {a, b, c}));
  ASSERT_EQ(mn->children, SET(SCCMetaNode*, {}));
  ASSERT_EQ(mn->parents,  SET(SCCMetaNode*, {}));

  ASSERT_EQ(a->meta_node, mn);
  ASSERT_EQ(b->meta_node, mn);
  ASSERT_EQ(c->meta_node, mn);
}

TEST_F(TagImplicationTest, ABC_SCC) {
  auto e = ctx.new_entity();
  e->add_tag(b);

  // first check that no tags match
  auto q = build_lit(c);
  auto res = query(ctx, *q);
  ASSERT_EQ(SET(Entity*, {}), res);
  delete q;

  // make an SCC of a,b,c
  a->imply(b);
  b->imply(c);
  c->imply(a);


  // // verify {e} matches for any of them
  for(auto node : {a, b, c}) {
    q = build_lit(node);
    q->debug_print();
    res = query(ctx, *q);
    ASSERT_EQ(SET(Entity*, {e}), res);
    delete q;
  }
}

TEST_F(TagImplicationTest, DiamondTest) {
  ASSERT_EQ(0, ctx.sink_meta_nodes.size());

  a->imply(b);
  ASSERT_EQ(1, ctx.sink_meta_nodes.size());

  a->imply(c);
  ASSERT_EQ(2, ctx.sink_meta_nodes.size());

  b->imply(d);
  ASSERT_EQ(2, ctx.sink_meta_nodes.size());

  c->imply(d);

  //      a
  //     / \
  //    b   c
  //     \ /
  //      d

  // a, b, c, d all are separate metanodes
  for(auto node : ctx.sink_meta_nodes) {
    node->print_tag_set(std::cerr); std::cerr << std::endl;
  }

  ASSERT_EQ(4, ctx.meta_nodes.size());
  ASSERT_EQ(1, ctx.sink_meta_nodes.size());

  // d -> a creates a cycle, collapse all
  d->imply(a);
  ASSERT_EQ(1, ctx.meta_nodes.size());
  ASSERT_EQ(1, ctx.sink_meta_nodes.size());
}

TEST_F(TagImplicationTest, TwoDiamondTest) {
  a->imply(b);
  a->imply(c);
  b->imply(d);
  c->imply(d);
  b->imply(e);

  //      a
  //     / \
  //    b   c
  //   / \ /
  //  e   d

  // a, b, c, d, e all are separate metanodes
  ASSERT_EQ(5, ctx.meta_nodes.size());
  ASSERT_EQ(2, ctx.sink_meta_nodes.size());

  // d -> a should collapse {a, b, c, d} into single node
  d->imply(a);

  // {a,b,c,d}
  //     |
  //     e

  ASSERT_EQ(2, ctx.meta_nodes.size());
  ASSERT_EQ(1, ctx.sink_meta_nodes.size());

  auto sink = *(ctx.sink_meta_nodes.begin());
  ASSERT_EQ(sink->tags, SET(Tag*, {e}));
  ASSERT_EQ(e->meta_node, sink);

  for(auto node : {a, b, c, d}) {
    ASSERT_EQ(node->meta_node->tags, SET(Tag*, {a, b, c, d}));
  }
}
