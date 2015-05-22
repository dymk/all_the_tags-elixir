#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <utility>

#include "entity.h"
#include "query.h"

struct Tag;

struct SCCMetaNode {
  int pre, post;

  std::unordered_set<SCCMetaNode*> children;
  std::unordered_set<SCCMetaNode*> parents;
  std::unordered_set<Tag*>         tags;

  SCCMetaNode()
    : pre(-1), post(-1) {}

  bool add_child(SCCMetaNode* c) {
    this->children.insert(c);
    return c->parents.insert(this).second;
  }

  bool remove_child(SCCMetaNode* c) {
    this->children.erase(c);
    return c->parents.erase(this) == 1;
  }

  int entity_count() const;
};

struct Context {
private:
  id_type last_tag_id;
  id_type last_entity_id;

  std::unordered_map<std::string, Tag*> value_to_tag;
  std::unordered_map<id_type,     Tag*> id_to_tag;

  std::unordered_map<id_type,  Entity*> id_to_entity;

  // meta nodes representing the DAG of tag implications
  std::unordered_set<SCCMetaNode*> meta_nodes;
  std::unordered_set<SCCMetaNode*> sink_meta_nodes;

public:
  std::unordered_set<Tag*> root_tags;

  Context() :
    last_tag_id(0),
    last_entity_id(0)
    // tag_graph_dirty(false)
    {}
  ~Context();

  // returns a new tag with value 'val'
  Tag *new_tag(const std::string& val);

  // returns a newly created entity
  Entity *new_entity();

  // look up tag by value or id
  Tag* tag_by_value(const std::string& val) const;
  Tag* tag_by_id(id_type tid) const;

  // notify the context that a node gained or
  // removed a parent, and to recalculate the tag tree's pre/post numbers
  void dirty_tag_parent_tree(Tag* dirtying_tag);

  // notify the context that 'dirtying_tag' gained/lost 'other' as an implied
  // tag. 'gained_imply' true if it now implies other, false if implication removed
  void dirty_tag_imply_dag(Tag* dirtying_tag, bool gained_imply, Tag* other);

  // look up entity by id
  Entity* entity_by_id(id_type eid) const;

  // calls 'match' with all entities that match the QueryClause
  template<class UnaryFunction>
  void query(const QueryClause *q, UnaryFunction match) const {

    auto iter = id_to_entity.begin();
    auto end = id_to_entity.end();
    for(; iter != end; iter++) {
      auto e = (*iter).second;

      if(q->matches_set(e->tags)) {
        match(e);
      }
    }
  }

  // context statistics
  size_t num_tags() const {
    return value_to_tag.size();
  }
  size_t num_entities() const {
    return id_to_entity.size();
  }
};

#endif
