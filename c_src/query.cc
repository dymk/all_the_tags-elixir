#include "query.h"

QueryClause *build_lit(Tag *tag) {
  QueryClause *clause = new QueryClauseLit(tag);
  for(auto child : tag->children) {
    // add parent chain to "or" tree (any of the parents match lit)
    clause = new QueryClauseOr(clause, build_lit(child));
  }

  return clause;
}
