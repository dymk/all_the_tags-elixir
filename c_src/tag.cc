#include "tag.h"
#include "context.h"

bool Tag::set_parent(Tag *parent) {
  if(!parent) return false;

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
  context->dirty_tag_graph(this);

  return true;
}
