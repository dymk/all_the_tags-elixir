#ifndef __TAGS_H__
#define __TAGS_H__

#include <string>
#include <unordered_set>
#include <cassert>

#include "id.h"

// needs forward declaration because C++ uses goddamn textual inclusion
struct Context;
struct SCCMetaNode;

struct Tag {
  id_type id;
  std::string value;

  Tag *parent;
  std::unordered_set<Tag*> children;

  // TODO: implement tag implications
  std::unordered_set<Tag*> implies;
  std::unordered_set<Tag*> implied_by;

  Context *context;
  // pre/post count of the tag in its parent/child tree
  int pre, post;

  // DAG SCC meta node that the tag belongs to
  SCCMetaNode *meta_node;

  // how many entities have this particular tag
  int _entity_count;

public:
  Tag(Context *context_, id_type _id, const std::string& _value) :
    id(_id), value(_value),
    parent(nullptr),
    context(context_),
    pre(-1),
    post(-1),
    meta_node(nullptr),
    _entity_count(0) {}

  bool set_parent(Tag *parent);

  bool unset_parent() {
    if(parent == nullptr) return false;
    this->parent->children.erase(this);
    this->parent = nullptr;
    pre = post = -1;
    return true;
  }

  bool is_or_descendent_of(const Tag* tag) const {
    if(pre == -1 || post == -1) {
      assert(pre == -1 && post == -1);
      return this == tag;
    }
    return pre >= tag->pre && tag->post >= post;
  }
  bool is_or_ancestor_of(const Tag* tag) const {
    if(pre == -1 || post == -1) {
      assert(pre == -1 && post == -1);
      return this == tag;
    }
    return pre <= tag->pre && tag->post <= post;
  }

  // this tag implies -> other tag
  bool imply(Tag *other);
  bool unimply(Tag *other);

  int entity_count() const {
    return _entity_count;
  }

  void inc_entity_count() {
    _entity_count++;
  }
  void dec_entity_count() {
    _entity_count--;
  }
};

#endif
