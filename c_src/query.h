#ifndef __QUERY_H__
#define __QUERY_H__

#include <unordered_set>
#include <algorithm>

#include "tag.h"

// root clause AST type
struct QueryClause {
  // returns true/false if the clause matches a given unordered set
  virtual bool matches_set(const std::unordered_set<Tag*>& tags) const = 0;
  virtual ~QueryClause() {}

  virtual int depth() const = 0;
  virtual int num_children() const = 0;
  virtual int entity_count() const = 0;
};

// clause negation
struct QueryClauseNot : public QueryClause {
  QueryClause* c;

  QueryClauseNot(QueryClause *c_) : c(c_) {}
  virtual ~QueryClauseNot() { delete c; }

  virtual bool matches_set(const std::unordered_set<Tag*>& tags) const {
    return !(c->matches_set(tags));
  }

  virtual int depth()        const { return c->depth() + 1;        }
  virtual int num_children() const { return c->num_children() + 1; }
  virtual int entity_count() const { return c->entity_count();     }
};

// logical clause operators
struct QueryClauseAnd : public QueryClause {
  QueryClause *l;
  QueryClause *r;

  QueryClauseAnd(QueryClause *l_, QueryClause *r_) : l(l_), r(r_) {}
  virtual ~QueryClauseAnd() { delete l; delete r; }

  virtual bool matches_set(const std::unordered_set<Tag*>& tags) const {
    return l->matches_set(tags) && r->matches_set(tags);
  }

  virtual int depth()        const { return std::max(l->depth(), r->depth()) + 1;      }
  virtual int num_children() const { return l->num_children() + r->num_children() + 1; }
  virtual int entity_count() const { return l->entity_count() + r->entity_count();     }
};
struct QueryClauseOr : public QueryClause {
  QueryClause *l;
  QueryClause *r;

  QueryClauseOr(QueryClause *l_, QueryClause *r_) : l(l_), r(r_) {}
  virtual ~QueryClauseOr() { delete l; delete r; }

  virtual bool matches_set(const std::unordered_set<Tag*>& tags) const {
    return l->matches_set(tags) || r->matches_set(tags);
  }

  virtual int depth()        const { return std::max(l->depth(), r->depth()) + 1;      }
  virtual int num_children() const { return l->num_children() + r->num_children() + 1; }
  virtual int entity_count() const { return l->entity_count() + r->entity_count();     }
};

// literal tag match
struct QueryClauseLit : public QueryClause {
  Tag *t;

  QueryClauseLit(Tag *t_) : t(t_) {}
  virtual ~QueryClauseLit() { t = nullptr; }
  virtual bool matches_set(const std::unordered_set<Tag*>& tags) const {
    return tags.find(t) != tags.end();
  }

  virtual int depth()        const { return 0; }
  virtual int num_children() const { return 0; }
  virtual int entity_count() const { return t->entity_count(); }
};

// represents an empty clause (matches everything)
struct QueryClauseAny : public QueryClause {
  virtual bool matches_set(const std::unordered_set<Tag*>& tags) const {
    (void)tags;
    return true;
  }

  QueryClauseAny() {}
  virtual ~QueryClauseAny() {}

  virtual int depth()        const { return 0; }
  virtual int num_children() const { return 0; }
  virtual int entity_count() const { return 0; }
};

QueryClause *build_lit(Tag *tag);

#endif /* __QUERY_H__ */
