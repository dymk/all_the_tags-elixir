#include "context.h"
#include "tag.h"

struct Tag;

int SCCMetaNode::entity_count() const {
  int sum = 0;
  for(auto t : tags) { sum += t->entity_count(); }
  return sum;
}

Context::~Context() {
  for(auto node : meta_nodes) {
    delete node;
  }
  for(auto pair : id_to_tag) {
    delete pair.second;
  }
  for(auto pair : id_to_entity) {
    delete pair.second;
  }
}

void Context::dirty_tag_parent_tree(Tag* dirtying_tag) {
  if(dirtying_tag->parent) {
    root_tags.erase(dirtying_tag);
  }
  else {
    root_tags.insert(dirtying_tag);
  }

  int counter = 0;
  std::function<void(Tag*)> recurse = [&](Tag *t) {
    t->pre = counter++;
    for(auto child : t->children) {
      recurse(child);
    }
    t->post = counter++;
  };

  for(Tag* t : root_tags) {
    recurse(t);
  }
}

bool path_between(SCCMetaNode *from, SCCMetaNode *to) {
  // do a DFS between 'from' and 'to'

  std::function<bool(SCCMetaNode*)> check = [&](SCCMetaNode *node) {
    if(node == to) return true;
    for(auto child : node->children) {
      if(check(child)) return true;
    }
    return false;
  };

  return check(from);
}

void Context::dirty_tag_imply_dag(Tag* tag, bool gained_imply, Tag* target) {
  // TODO: optimize this a lot more
  // for now, destroy all the metanodes and recreate the entire graph

  if(gained_imply) {
    // tag now implies target
    SCCMetaNode
      *tag_mn    = tag->meta_node,
      *target_mn = target->meta_node;


    if(!tag_mn || !target_mn) {
      if(!tag_mn) {
        tag->meta_node = tag_mn = new SCCMetaNode();
        tag_mn->tags.insert(tag);
        meta_nodes.insert(tag_mn);
      }
      if(!target_mn) {
        target->meta_node = target_mn = new SCCMetaNode();
        target_mn->tags.insert(target);
        meta_nodes.insert(target_mn);
      }

      assert(tag->meta_node->add_child(target->meta_node));
      sink_meta_nodes.erase(tag_mn);
      sink_meta_nodes.insert(target_mn);
    }
    else {
      // both tag and target already have a metanode
      // check if target has a path to tag
      // if it does, collapse the whole thing into a
      // single metanode
      assert(target_mn && tag_mn);
      if(path_between(target_mn, tag_mn)) {
        assert(false && "TODO: implement if there *is* a path between the two");
      }
      else {
        // no path between the two, won't create a cycle
        // can safely add link between the two
        tag_mn->add_child(target_mn);
      }
    }
  }
  else {
    assert(false && "TODO: implement implication removal");
  }
}

Tag* Context::new_tag(const std::string& val) {
  std::string tmp = val;
  std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);

  if(tag_by_value(val)) {
    return nullptr;
  }

  auto t = new Tag(this, last_tag_id++, val);

  // tags.push_back(t); // record in master tags list
  root_tags.insert(t);
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
