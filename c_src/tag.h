#ifndef __TAGS_H__
#define __TAGS_H__

#include <string>
#include <unordered_set>

#include "id.h"

struct Tag {
  id_type id;
  std::string value;

  Tag *parent;
  std::unordered_set<Tag*> children;

  // TODO: implement tag implications
  std::unordered_set<Tag*> implies;
  std::unordered_set<Tag*> implied_by;

  Tag(id_type _id, const std::string& _value) :
    id(_id), value(_value), parent(nullptr) {}

  bool set_parent(Tag *parent) {
    // check for cycles
    auto p = parent;
    while(p->parent) {
      if(p->parent == this) return false;
      p = p->parent;
    }


    // remove ourselves from current parent's children list (if we have one)
    if(this->parent) {
      this->parent->children.erase(this);
    }

    this->parent = parent;
    parent->children.insert(this);

    return true;
  }

  // this tag implies -> other tag
  bool imply(Tag *other) {
    other->implied_by.insert(this);
    return implies.insert(other).second;
  }

  bool unimply(Tag *other) {
    other->implied_by.erase(this);
    return implies.erase(other) == 1;
  }
};

#endif
