#ifndef __QUERY_H__
#define __QUERY_H__

#include <unordered_set>

#include "tag.h"

// root clause AST type
struct QueryClause {
  // returns true/false if the clause matches a given unordered set
  virtual bool matches_set(const std::unordered_set<Tag*>& tags) const = 0;
  virtual ~QueryClause() {}
};

// clause negation
struct QueryClauseNot : public QueryClause {
  QueryClause* c;

  QueryClauseNot(QueryClause *c_) : c(c_) {}
  virtual ~QueryClauseNot() { delete c; }

  virtual bool matches_set(const std::unordered_set<Tag*>& tags) const {
    return !(c->matches_set(tags));
  }
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
};
struct QueryClauseOr : public QueryClause {
  QueryClause *l;
  QueryClause *r;

  QueryClauseOr(QueryClause *l_, QueryClause *r_) : l(l_), r(r_) {}
  virtual ~QueryClauseOr() { delete l; delete r; }

  virtual bool matches_set(const std::unordered_set<Tag*>& tags) const {
    return l->matches_set(tags) || r->matches_set(tags);
  }
};

// literal tag match
struct QueryClauseLit : public QueryClause {
  Tag *t;

  QueryClauseLit(Tag *t_) : t(t_) {}
  virtual ~QueryClauseLit() { t = nullptr; }
  virtual bool matches_set(const std::unordered_set<Tag*>& tags) const {
    return tags.find(t) != tags.end();
  }
};

// represents an empty clause (matches everything)
struct QueryClauseAny : public QueryClause {
  virtual bool matches_set(const std::unordered_set<Tag*>& tags) const {
    (void)tags;
    return true;
  }

  QueryClauseAny() {}
  virtual ~QueryClauseAny() {}
};

#endif /* __QUERY_H__ */
