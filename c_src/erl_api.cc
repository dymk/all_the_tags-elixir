#include <iostream>
#include <cassert>
#include <cstring>

#include "query.h"
#include "context.h"
#include "erl_helpers.h"

static bool debug = false;

struct PubContext {
  Context context;
  // cache mappings from tag id to its value's erlang binary
  std::unordered_map<id_type, Tag*> tag_binary_cache;
};

static int enif_binary_or_list_to_string(ErlNifEnv *env, ERL_NIF_TERM term, char *buf, unsigned int buflen) {
  if(enif_is_binary(env, term)) {
    ErlNifBinary bin;
    if(!enif_inspect_binary(env, term, &bin)) {
      return -1;
    }

    if(bin.size >= buflen) {
      // can't fit buffer into buflen
      return -1;
    }

    // can fit binary into buffer
    // TODO: figure out way to just return the binary buffer rather than copy
    strncpy(buf, (const char*)bin.data, bin.size);
    buf[bin.size] = '\0';
    return bin.size;
  }
  else if(enif_is_list(env, term)) {
    return enif_get_string(env, term, buf, buflen, ERL_NIF_LATIN1);
  }

  return -1;
}

ErlNifResourceType *context_type = nullptr;
static void context_resource_cleanup(ErlNifEnv *env, void *arg) {
  UNUSED(env);
  if(debug) std::cout << "native: cleaning up resource (STUB)" << std::endl;

  assert(arg);
  PubContext *pc = (PubContext*) arg;
  pc->~PubContext();
}

// main entrypoint that erlang talks to to perform queries on
// a tag/entity collection context
ERL_FUNC(init_lib) {
  UNUSED(argc);
  UNUSED(argv);
  if(debug) std::cout << "native: init native tags api" << std::endl;

  if(context_type != nullptr) {
    if(debug) std::cerr << "native: already allocated resource type; double init?" << std::endl;
    return A_ERR(env);
  }

  ErlNifResourceFlags flags = (ErlNifResourceFlags)
  (ERL_NIF_RT_CREATE | ERL_NIF_RT_TAKEOVER);
  context_type = enif_open_resource_type(env,
    nullptr, "tags_nif_handle",
    &context_resource_cleanup,
    flags, nullptr);

  if(context_type == nullptr) {
    if(debug) std::cerr << "native: couldn't make resource type" << std::endl;
    return A_ERR(env);
  }

  return A_OK(env);
}

ERL_FUNC(new_) {
  UNUSED(argv);
  ENSURE_ARG(argc == 0);

  assert(context_type);

  PubContext *pc = (PubContext*)enif_alloc_resource(context_type, sizeof(PubContext));
  if(pc == nullptr) {
    if(debug) std::cout << "native: could not allocate resource" << std::endl;
    return A_ERR(env);
  }
  else {
    if(debug) std::cout << "native: allocated a resource (" << sizeof(PubContext) << " bytes)" << std::endl;
  }

  // call ctor on erlang allocated memory
  new(pc) PubContext();

  auto term = enif_make_resource(env, pc);
  enif_release_resource(pc);
  return enif_make_tuple2(env, A_OK(env), term);
}

ERL_FUNC(new_tag) {
  if(argc != 2) {
    if(debug) std::cout << "native: new_tag called without 2 args: " << argc << std::endl;
    return enif_make_badarg(env);
  }

  PubContext *pc = nullptr;
  ENSURE_ARG(enif_get_resource(env, argv[0], context_type, (void**)&pc));
  assert(pc);

  char value[100];
  ENSURE_ARG(enif_binary_or_list_to_string(env, argv[1], value, 100) > 0);
  std::string s_value(value);

  // allocate new tag, insert into the
  auto t = pc->context.new_tag(s_value);
  if(!t) {
    return A_ERR(env);
  }

  // cache a copy of a erlang bitarray representing the tag's name

  if(debug) std::cout << "native: added tag with value " << s_value << std::endl;
  return A_OK(env);
}

ERL_FUNC(num_tags) {
  ENSURE_ARG(argc == 1);

  PubContext *pc = nullptr;
  ENSURE_ARG(enif_get_resource(env, argv[0], context_type, (void**)&pc));
  assert(pc);

  return enif_make_uint(env, pc->context.num_tags());
}

ERL_FUNC(new_entity) {
  ENSURE_ARG(argc == 1);

  PubContext *pc = nullptr;
  ENSURE_ARG(enif_get_resource(env, argv[0], context_type, (void**)&pc));
  assert(pc);

  auto e = pc->context.new_entity();
  return enif_make_uint(env, e->id);
}

ERL_FUNC(num_entities) {
  ENSURE_ARG(argc == 1);

  PubContext *pc = nullptr;
  ENSURE_ARG(enif_get_resource(env, argv[0], context_type, (void**)&pc));
  assert(pc);

  return enif_make_uint(env, pc->context.num_entities());
}

// {handle, entity_id, tag_value}
ERL_FUNC(add_tag) {
  ENSURE_ARG(argc == 3);

  PubContext *pc = nullptr;
  ENSURE_ARG(enif_get_resource(env, argv[0], context_type, (void**)&pc));
  assert(pc);
  Context *context = &(pc->context);

  id_type entity_id;
  ENSURE_ARG(enif_get_uint(env, argv[1], &entity_id));

  char tag_val[100];
  ENSURE_ARG(enif_binary_or_list_to_string(env, argv[2], tag_val, 100) > 0);
  std::string stag_val(tag_val);

  // look up tag based on value given
  auto tag = context->tag_by_value(stag_val);
  if(!tag) return A_ERR(env);

  auto entity = context->entity_by_id(entity_id);
  if(!entity) return A_ERR(env);

  if(!entity->add_tag(tag)) return A_ERR(env);

  return A_OK(env);
}

ERL_FUNC(entity_tags) {
  ENSURE_ARG(argc == 2);

  PubContext *pc = nullptr;
  ENSURE_ARG(enif_get_resource(env, argv[0], context_type, (void**)&pc));
  assert(pc);

  id_type entity_id;
  ENSURE_ARG(enif_get_uint(env, argv[1], &entity_id));
  auto entity = pc->context.entity_by_id(entity_id);
  if(!entity) return A_ERR(env);

  ERL_NIF_TERM res_list = enif_make_list(env, 0); // start with empty list
  {
    // TODO: cache the tag value as a binary somewhere and reuse it
    auto itr = entity->tags.begin();
    auto end = entity->tags.end();

    for(; itr != end; itr++) {
      auto tag = *itr;
      auto tv = tag->value;

      // allocate a new binary
      ErlNifBinary bin;
      if(!enif_alloc_binary(tv.size(), &bin)) return A_ERR(env);
      // copy c string value into the binary
      assert(bin.size == tv.size());
      strncpy((char*)bin.data, tv.c_str(), bin.size);

      auto term = enif_make_binary(env, &bin);
      res_list = enif_make_list_cell(env, term, res_list);
    }
  }

  return enif_make_tuple2(env, A_OK(env), res_list);
}

// converts erlang clause AST into native AST representation
static QueryClause *build_clause(ErlNifEnv *env, const ERL_NIF_TERM term, const Context* c) {
  if(enif_is_list(env, term) || enif_is_binary(env, term)) {
    // literal tag string
    // convert to string, get tag by that ID
    char tag_val[100];
    if(enif_binary_or_list_to_string(env, term, tag_val, 100) <= 0) {
      // couldn't convert
      return nullptr;
    }
    std::string stag_val(tag_val);
    auto tag = c->tag_by_value(stag_val);
    if(!tag) return nullptr;

    return new QueryClauseLit(tag);
  }
  else if(enif_is_tuple(env, term)) {
    // tuple in the form of {:and, a, b}
    // or {:or, a, b}
    const ERL_NIF_TERM *elems;
    int arity;
    if(!enif_get_tuple(env, term, &arity, &elems)) {
      return nullptr;
    }
    // first member must be an atom
    auto first = elems[0];
    if(!enif_is_atom(env, first)) return nullptr;

    if(arity == 2) {
      // arity of 2 - must be a "not"
      bool matches_not = enif_compare(first, enif_make_atom(env, "not")) == 0;
      if(!matches_not) return nullptr;

      auto e = build_clause(env, elems[1], c);
      if(!e) return nullptr;

      return new QueryClauseNot(e);
    }
    else if(arity == 3) {
      // arity of 3 - "and" or "or"
      bool matches_and = enif_compare(first, enif_make_atom(env, "and")) == 0;
      bool matches_or  = enif_compare(first, enif_make_atom(env, "or"))  == 0;
      assert(!(matches_and && matches_or));
      if(!matches_and && !matches_or) {
        return nullptr;
      }

      // AND clause
      auto l = build_clause(env, elems[1], c);
      if(!l) return nullptr;
      auto r = build_clause(env, elems[2], c);
      if(!r) return nullptr;

      if(matches_and) {
        return new QueryClauseAnd(l, r);
      }
      else {
        return new QueryClauseOr(l, r);
      }
    }
    else {
      // incorrect arity (must be 2 or 3) - no matches
      return nullptr;
    }
  }
  else if(enif_is_atom(env, term)) {
    // 'nil' represents empty query - matches everything
    bool matches_nil = enif_compare(term, enif_make_atom(env, "nil")) == 0;
    if(!matches_nil) return nullptr;

    return new QueryClauseAny();
  }
  else {
    // no other matches
    return nullptr;
  }

  assert(false && "impossible");
}

// {handle, clause}
ERL_FUNC(do_query) {
  ENSURE_ARG(argc == 2);

  PubContext *pc = nullptr;
  ENSURE_ARG(enif_get_resource(env, argv[0], context_type, (void**)&pc));
  assert(pc);

  if(debug) std::cout << "native: do_query called" << std::endl;

  QueryClause *c = build_clause(env, argv[1], &(pc->context));
  if(c == nullptr) { return A_ERR(env); }

  ERL_NIF_TERM res_list = enif_make_list(env, 0); // start with empty list

  pc->context.query(c, [&](const Entity* e) {
    auto term = enif_make_uint(env, e->id);
    res_list = enif_make_list_cell(env, term, res_list);
  });

  delete c;

  return enif_make_tuple2(env, A_OK(env), res_list);
}


ERL_FUNC(make_tag_parent) {
  ENSURE_ARG(argc == 3);

  PubContext *pc = nullptr;
  ENSURE_ARG(enif_get_resource(env, argv[0], context_type, (void**)&pc));
  assert(pc);
  Context* context = &(pc->context);

  char tv[100];
  ENSURE_ARG(enif_binary_or_list_to_string(env, argv[1], tv, 100) > 0);
  std::string parent_tag_val(tv);
  auto parent = context->tag_by_value(parent_tag_val);
  if(!parent) return A_ERR(env);

  ENSURE_ARG(enif_binary_or_list_to_string(env, argv[2], tv, 100) > 0);
  std::string child_tag_val(tv);
  auto child = context->tag_by_value(child_tag_val);
  if(!child) return A_ERR(env);

  // set parent
  child->parent = parent;

  return A_OK(env);
}

static ErlNifFunc nif_funcs[] = {
  {"init_lib",        0, init_lib,      0}, // private
  {"new",             0, new_,          0},
  {"new_tag" ,        2, new_tag,       0},
  {"num_tags",        1, num_tags,      0},
  {"new_entity",      1, new_entity,    0},
  {"num_entities",    1, num_entities,  0},
  {"add_tag",         3, add_tag,       0},
  {"entity_tags",     2, entity_tags,   0},
  {"do_query",        2, do_query,      0},
  {"make_tag_parent", 3, make_tag_parent, 0}
};

ERL_NIF_INIT(Elixir.AllTheTags, nif_funcs, NULL, NULL, NULL, NULL)
