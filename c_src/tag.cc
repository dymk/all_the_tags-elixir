#include "tag.h"
#include "context.h"

bool Tag::set_parent(Tag *parent) {
  if(!parent) return false;
  if(this->parent && this->parent == parent) return false;

  // check for cycles
  auto p = parent;
  while(p->parent) {
    if(p->parent == this) return false;
    p = p->parent;
  }

  // remove ourselves from current parent's children list (if we have one)
  unset_parent();

  this->parent = parent;
  parent->children.insert(this);

  // notify the context the tag graph (pre/post) needs recalculating
  context->dirty_tag_parent_tree(this);

  return true;
}

bool Tag::imply(Tag *other) {
  auto a = other->implied_by.insert(this).second;
  auto b = implies.insert(other).second;
  assert(a == b);

  if(a) context->dirty_tag_imply_dag(this, true, other);

  return a;
}
bool Tag::unimply(Tag *other) {
  auto a = other->implied_by.erase(this) == 1;
  auto b = implies.erase(other) == 1;
  assert(a == b);

  if(a) context->dirty_tag_imply_dag(this, false, other);

  return a;
}
