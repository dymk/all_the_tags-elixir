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

  // std::vector<Tag*>    tags; // TODO: probably don't need this
  // std::vector<Entity*> entities;

  std::unordered_map<std::string, Tag*> value_to_tag;
  std::unordered_map<id_type,     Tag*> id_to_tag;

  std::unordered_map<id_type,  Entity*> id_to_entity;

public:
  Context() : last_tag_id(0), last_entity_id(0)
  {}

  ~Context() {
    {
      auto itr = id_to_tag.begin();
      auto end = id_to_tag.end();
      for(; itr != end; itr++) {
        delete (*itr).second;
      }
    }
    {
      auto itr = id_to_entity.begin();
      auto end = id_to_entity.end();
      for(; itr != end; itr++) {
        delete (*itr).second;
      }
    }
  }

  Tag *new_tag(const std::string& val) {
    std::string tmp = val;
    std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);

    if(tag_by_value(val)) {
      return nullptr;
    }

    auto t = new Tag(last_tag_id++, val);

    // tags.push_back(t); // record in master tags list
    value_to_tag.insert(std::make_pair(tmp, t)); // all values
    id_to_tag.insert(std::make_pair(t->id, t));

    return t;
  }

  Entity *new_entity() {
    auto e = new Entity(last_entity_id++);
    id_to_entity.insert(std::make_pair(e->id, e));
    return e;
  }

  // tag lookup functions
  Tag* tag_by_value(const std::string& val) const {
    // downcase the string
    std::string tmp = val;
    std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);

    auto iter = value_to_tag.find(tmp);
    if(iter != value_to_tag.end()) {
      return (*iter).second;
    }
    else {
      return nullptr;
    }
  }
  Tag* tag_by_id(id_type tid) const {
    auto iter = id_to_tag.find(tid);
    if(iter != id_to_tag.end()) {
      return (*iter).second;
    }
    else {
      return nullptr;
    }
  }

  // entity lookup functions
  Entity* entity_by_id(id_type eid) const {
    auto iter = id_to_entity.find(eid);
    if(iter != id_to_entity.end()) {
      return (*iter).second;
    }
    else {
      return nullptr;
    }
  }

  template<class UnaryFunction>
  void query(const QueryClause *q, UnaryFunction match) {

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
