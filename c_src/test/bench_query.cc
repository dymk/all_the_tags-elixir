#include <hayai.hpp>
#include "test_helper.h"

class BenchQuery : public ::hayai::Fixture
{
public:
  std::unordered_set<Tag*> tags;
  std::unordered_set<QueryClause*> queries;
  Context c;

  Tag *foo, *bar, *baz, *qux;
  QueryClause *q_foo ,*q_bar ,*q_baz ,*q_qux;

  Entity* e1;


  virtual void SetUp() {
    foo = c.new_tag("foo");
    bar = c.new_tag("bar");
    baz = c.new_tag("baz");
    qux = c.new_tag("qux");
    tags = SET(Tag*, {foo, bar, baz, qux});

    // qux is parent of baz
    // anything tagged with baz will appear in a query for qux
    baz->set_parent(qux);

    // create a few dummy entities
    for(int i = 0; i < 1000; i++) { c.new_entity(); }

    e1 = c.new_entity();
    e1->add_tag(foo);
    e1->add_tag(bar);

    // 1000 with tag "baz"
    for(int i = 0; i < 1000; i++) { c.new_entity()->add_tag(baz); }

    // 1000 with tag "qux"
    for(int i = 0; i < 1000; i++) { c.new_entity()->add_tag(qux); }

    for(int i = 0; i < 1000; i++) { c.new_entity(); }

    // 200 entities with no tags
    // 1 entity with "foo" and "bar"
    q_foo = build_lit(foo);
    q_bar = build_lit(bar);
    q_baz = build_lit(baz);
    q_qux = build_lit(qux);

    queries = SET(QueryClause*, {q_foo, q_bar, q_baz, q_qux});
  }

  virtual void TearDown() {
    for(auto t : queries) {
      delete t;
    }
  }
};

BENCHMARK(BenchQuery, NewEntity500, 10, 50) {
  Context c;
  for(int i = 0; i < 500; i++) { c.new_entity(); }
}
BENCHMARK(BenchQuery, NewTag500, 10, 50) {
  Context c;

  char name[100];
  for(int i = 0; i < 500; i++) {
    sprintf(name, "tag%d", i);
    c.new_tag(name);
  }
}

BENCHMARK_F(BenchQuery, DoQuery1, 10, 100) {
  auto set = SET(Entity*, {});

  c.query(q_foo, [&](Entity* e) {
    set.insert(e);
  });

  assert(set == SET(Entity*, {e1}));
}

BENCHMARK_F(BenchQuery, QueryForBaz, 10, 100) {
  int count = 0;
  c.query(q_baz, [&](Entity* e) {
    count++;
  });

  assert(count == 1000);
}

BENCHMARK_F(BenchQuery, QueryForQux, 10, 100) {
  int count = 0;
  c.query(q_qux, [&](Entity* e) {
    count++;
  });

  // matches all with tag qux and baz
  assert(count == 2000);
}
