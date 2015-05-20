#ifndef __TAGS_H__
#define __TAGS_H__

#include <string>

#include "id.h"

struct Tag {
  id_type id;
  std::string value;

  Tag(id_type _id, const std::string& _value) :
    id(_id), value(_value) {}
};

#endif
