#ifndef __QUERY_H__
#define __QUERY_H__

#include <unordered_set>
#include <algorithm>
#include <iostream>

#include "tag.h"

struct QueryClause;
struct QueryClauseBin;
struct QueryClauseNot;

QueryClause    *build_lit(Tag *tag);
QueryClauseBin *build_and(QueryClause *r, QueryClause *l);
QueryClauseBin *build_or (QueryClause *r, QueryClause *l);
QueryClauseNot *build_not(QueryClause *c);
QueryClause    *optimize(QueryClause *clause);

// root clause AST type
struct QueryClause {
  // returns true/false if the clause matches a given unordered set
  virtual bool matches_set(const std::unordered_set<Tag*>& tags) const = 0;
  virtual ~QueryClause() {}

  virtual int depth() const = 0;
  virtual int num_children() const = 0;
  virtual int entity_count() const = 0;
  virtual QueryClause *dup() const = 0;

  virtual void debug_print(int indent = 0) const = 0;
protected:
  void print_indent(int indent) const {
    for(int i = 0; i < indent; i++) {
      std::cerr << "  ";
    }
  }
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
  virtual QueryClauseNot *dup() const {
    return new QueryClauseNot(c->dup());
  }

  virtual void debug_print(int indent = 0) const {
    print_indent(indent);
    std::cerr << "not ->" << std::endl;
    c->debug_print(indent + 1);
  }

};

// logical clause operators
enum QueryClauseBinType {
  QueryClauseAnd,
  QueryClauseOr
};
struct QueryClauseBin : public QueryClause {
  QueryClauseBinType type;
  QueryClause *l;
  QueryClause *r;

  QueryClauseBin(QueryClauseBinType type_, QueryClause *l_, QueryClause *r_) :
    type(type_), l(l_), r(r_) {}

  virtual ~QueryClauseBin() {
    if(l) delete l;
    if(r) delete r;
  }

  virtual bool matches_set(const std::unordered_set<Tag*>& tags) const {
    if(type == QueryClauseAnd) {
      return l->matches_set(tags) && r->matches_set(tags);
    }
    else {
      return r->matches_set(tags) || l->matches_set(tags);
    }
  }

  virtual int depth()        const { return std::max(l->depth(), r->depth()) + 1;      }
  virtual int num_children() const { return l->num_children() + r->num_children() + 1; }
  virtual int entity_count() const { return l->entity_count() + r->entity_count();     }
  virtual QueryClauseBin *dup() const {
    return new QueryClauseBin(type, l->dup(), r->dup());
  }

  virtual void debug_print(int indent = 0) const {
    print_indent(indent);
    std::cerr << "bin(" << (type == QueryClauseAnd ? "and" : "or") << ") ->" << std::endl;
    l->debug_print(indent + 1);
    r->debug_print(indent + 1);
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

  virtual int depth()        const { return 0; }
  virtual int num_children() const { return 0; }
  virtual int entity_count() const { return t->entity_count(); }
  virtual QueryClauseLit *dup() const {
    return new QueryClauseLit(t);
  }

  virtual void debug_print(int indent = 0) const {
    print_indent(indent);
    std::cerr << "lit -> " << t->value << " (" << t->entity_count() <<")"<< std::endl;
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

  virtual int depth()        const { return 0; }
  virtual int num_children() const { return 0; }
  virtual int entity_count() const { return 0; }

  virtual void debug_print(int indent = 0) const {
    print_indent(indent);
    std::cerr << "any" << std::endl;
  }

  virtual QueryClauseAny* dup() const {
    return new QueryClauseAny();
  }
};

#endif /* __QUERY_H__ */
