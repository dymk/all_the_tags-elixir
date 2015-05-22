#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <utility>

#include "tag.h"
#include "entity.h"
#include "query.h"

struct Context {
private:
  id_type last_tag_id;
  id_type last_entity_id;

  std::unordered_map<std::string, Tag*> value_to_tag;
  std::unordered_map<id_type,     Tag*> id_to_tag;

  std::unordered_map<id_type,  Entity*> id_to_entity;

public:
  Context() : last_tag_id(0), last_entity_id(0) {}
  ~Context();

  // returns a new tag with value 'val'
  Tag *new_tag(const std::string& val);

  // returns a newly created entity
  Entity *new_entity();

  // look up tag by value or id
  Tag* tag_by_value(const std::string& val) const;
  Tag* tag_by_id(id_type tid) const;

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
