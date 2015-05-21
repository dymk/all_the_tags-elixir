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
};

#endif
