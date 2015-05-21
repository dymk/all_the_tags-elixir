#include "context.h"

Context::~Context() {
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

Tag* Context::new_tag(const std::string& val) {
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

Entity* Context::new_entity() {
  auto e = new Entity(last_entity_id++);
  id_to_entity.insert(std::make_pair(e->id, e));
  return e;
}

Tag* Context::tag_by_value(const std::string& val) const {
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
Tag* Context::tag_by_id(id_type tid) const {
  auto iter = id_to_tag.find(tid);
  if(iter != id_to_tag.end()) {
    return (*iter).second;
  }
  else {
    return nullptr;
  }
}

// entity lookup functions
Entity* Context::entity_by_id(id_type eid) const {
  auto iter = id_to_entity.find(eid);
  if(iter != id_to_entity.end()) {
    return (*iter).second;
  }
  else {
    return nullptr;
  }
}
