#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

#include "tag.h"
#include "entity.h"

struct Context {
private:
  id_type last_tag_id;

  // std::vector<Tag*>    tags; // TODO: probably don't need this
  std::vector<Entity*> entities;

  std::unordered_map<std::string, Tag*> value_to_tag;
  std::unordered_map<id_type,     Tag*> id_to_tag;

public:
  Context() : last_tag_id(0)
  {}

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

  Tag *insert_new_tag(const std::string& val) {
    std::string tmp = val;
    std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);

    auto t = new Tag(last_tag_id++, val);

    // tags.push_back(t); // record in master tags list
    value_to_tag.insert(std::make_pair(tmp, t)); // all values
    id_to_tag.insert(std::make_pair(t->id, t));

    return t;
  }

  size_t num_tags() const {
    return value_to_tag.size();
  }
};

#endif
