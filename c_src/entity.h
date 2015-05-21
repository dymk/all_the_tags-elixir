#ifndef __ENTITY_H__
#define __ENTITY_H__

#include <unordered_set>
#include "id.h"

// represents an entity that can be tagged
// is represented by a unique ID
struct Entity {
  std::unordered_set<Tag*> tags;
  id_type id;

  Entity(id_type _id) : id(_id) {}

  // add tag to the entity
  // returns:
  //  - true: tag was added
  //  - false: tag arleady on this entity
  bool add_tag(Tag* t) {
    return tags.insert(t).second;
  }
};

#endif