#include "query.h"
#include "context.h"

#include <stack>
#include <queue>
#include <vector>

int QueryClauseMetaNode::entity_count() const {
  return node->entity_count();
}
void QueryClauseMetaNode::debug_print(int indent) const {
  print_indent(indent);
  std::cerr << "meta(" << entity_count() << ") -> (";
  bool first = true;
  for(auto tag : node->tags) {
    if(!first) std::cerr << ", ";
    first = false;
    std::cerr << tag->value;
  }
  std::cerr << ")" << std::endl;
}

QueryClause *build_lit(Tag *tag) {
  QueryClause *clause = new QueryClauseLit(tag);
  for(auto child : tag->children) {
    // add parent chain to "or" tree (any of the parents match lit)
    clause = build_or(clause, build_lit(child));
  }

  // TODO: if a parent has a ton of children, might be better
  // to do more expensive range-based check?
  if(tag->meta_node) {
    std::unordered_set<SCCMetaNode*> meta_nodes;

    std::function<void(SCCMetaNode*)> recurse = [&](SCCMetaNode *node) {
      meta_nodes.insert(node);
      for(auto parent : node->parents) {
        recurse(parent);
      }
    };

    // collect all reachable metanodes from this node into meta_nodes
    recurse(tag->meta_node);

    for(auto node : meta_nodes) {
      clause = build_or(clause, new QueryClauseMetaNode(node));
    }
  }

  return clause;
}

QueryClauseBin *build_and(QueryClause *r, QueryClause *l) {
  return new QueryClauseBin(QueryClauseAnd, r, l);
}
QueryClauseBin *build_or (QueryClause *r, QueryClause *l) {
  return new QueryClauseBin(QueryClauseOr, r, l);
}
QueryClauseNot *build_not(QueryClause *c) {
  return new QueryClauseNot(c);
}

// forward decls
QueryClause* hc_tree_optimize(QueryClauseBin* clause);

QueryClause *optimize(QueryClause *clause) {
  // test for tree restructuring
  auto cast_bin = dynamic_cast<QueryClauseBin*>(clause);
  // not a binary clause; ignore
  if(!cast_bin) {
    return clause;
  }
  else {
    return hc_tree_optimize(cast_bin);
  }
}

class QueryClauseCompare
{
  QueryClauseBinType type;
public:
  QueryClauseCompare(QueryClauseBinType type_) : type(type_) {}
  bool operator() (const QueryClause* l, const QueryClause* r) const {
    if(type == QueryClauseAnd) {
      // reverse priority (lower entites at top) for left
      return l->entity_count() > r->entity_count();
    }
    else {
      return l->entity_count() < r->entity_count();
    }
  }
};

QueryClause* hc_tree_optimize(QueryClauseBin* clause) {
  // used when building the HC tree
  std::priority_queue<
    QueryClause*,
    std::vector<QueryClause*>,
    QueryClauseCompare> queued_leafs(QueryClauseCompare(clause->type));

  std::vector<QueryClause*> unsorted_leafs;

  std::stack<QueryClauseBin*> bin_nodes;
  std::stack<QueryClauseBin*> recycle_bin;

  // push a given query clause to the bin_nodes or the leafs
  // queue depending on its type
  auto enqueue_node = [&](QueryClause* c) {
    auto casted = dynamic_cast<QueryClauseBin*>(c);
    if(casted && casted->type == clause->type) {
      bin_nodes.push(casted);
    }
    else {
      unsorted_leafs.push_back(c);
    }
  };

  // return a recycled (or new) binary node
  auto get_bin_node = [&]() {
    if(recycle_bin.size()) {
      auto ret = recycle_bin.top();
      recycle_bin.pop();
      assert(ret->type == clause->type);
      return ret;
    }
    else {
      return new QueryClauseBin(clause->type, nullptr, nullptr);
    }
  };

  bin_nodes.push(clause);
  while(bin_nodes.size()) {
    auto top = bin_nodes.top();
    bin_nodes.pop();

    // get leafs that are also of clause type
    enqueue_node(top->l);
    enqueue_node(top->r);
    top->r = top->l = nullptr;
    recycle_bin.push(top);
  }

  // recursivly optimize all the leafs (which reside in unsorted_leafs)
  for(auto&& leaf : unsorted_leafs) {
    auto old_count = leaf->entity_count();
    leaf = optimize(leaf);
    assert(old_count == leaf->entity_count());
  }

  // insert all the leafs into the priority queue
  for(auto leaf : unsorted_leafs) {
    queued_leafs.push(leaf);
  }

  // build the huffman coding tree
  while(queued_leafs.size() > 1) {
    auto first  = queued_leafs.top(); queued_leafs.pop();
    auto second = queued_leafs.top(); queued_leafs.pop();
    auto parent = get_bin_node();
    parent->l = first;
    parent->r = second;
    queued_leafs.push(parent);
  }

  // delete the unused recycled nodes
  while(recycle_bin.size()) {
    delete recycle_bin.top();
    recycle_bin.pop();
  }

  // last element in the
  return queued_leafs.top();
}
