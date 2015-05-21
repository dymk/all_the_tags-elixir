#include <iostream>
#include <cassert>
#include <cstring>
#include <numeric>

#include "query.h"
#include "context.h"
#include "erl_api_helpers.h"

static bool debug = false;

ErlNifResourceType *context_type = nullptr;
static void context_resource_cleanup(ErlNifEnv *env, void *arg) {
  UNUSED(env);
  if(debug) std::cout << "native: cleaning up resource (STUB)" << std::endl;

  assert(arg);
  Context *context = (Context*) arg;
  context->~Context();
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
  Context *context = (Context*)enif_alloc_resource(context_type, sizeof(Context));
  if(context == nullptr) {
    if(debug) std::cout << "native: could not allocate resource" << std::endl;
    return A_ERR(env);
  }
  else {
    if(debug) std::cout << "native: allocated a resource (" << sizeof(Context) << " bytes)" << std::endl;
  }

  // call ctor on erlang allocated memory
  new(context) Context();

  auto term = enif_make_resource(env, context);
  enif_release_resource(context);
  return enif_make_tuple2(env, A_OK(env), term);
}

ERL_FUNC(new_tag) {
  ENSURE_ARG(argc == 2);
  ENSURE_CONTEXT(env, argv[0]);

  char value[100];
  ENSURE_ARG(enif_binary_or_list_to_string(env, argv[1], value, 100) > 0);
  std::string s_value(value);

  // allocate new tag, insert into the
  auto t = context->new_tag(s_value);
  if(!t) {
    return A_ERR(env);
  }

  if(debug) std::cout << "native: added tag with value " << s_value << std::endl;
  return A_OK(env);
}

ERL_FUNC(num_tags) {
  ENSURE_ARG(argc == 1);
  ENSURE_CONTEXT(env, argv[0]);

  return enif_make_uint(env, context->num_tags());
}

ERL_FUNC(new_entity) {
  ENSURE_ARG(argc == 1);
  ENSURE_CONTEXT(env, argv[0]);

  auto e = context->new_entity();
  return enif_make_uint(env, e->id);
}

ERL_FUNC(num_entities) {
  ENSURE_ARG(argc == 1);

  Context *context = nullptr;
  ENSURE_ARG(enif_get_resource(env, argv[0], context_type, (void**)&context));
  assert(context);

  return enif_make_uint(env, context->num_entities());
}

// {handle, entity_id, tag_value}
ERL_FUNC(add_tag) {
  ENSURE_ARG(argc == 3);
  ENSURE_CONTEXT(env, argv[0]);

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
  ENSURE_CONTEXT(env, argv[0]);

  id_type entity_id;
  ENSURE_ARG(enif_get_uint(env, argv[1], &entity_id));
  auto entity = context->entity_by_id(entity_id);
  if(!entity) return A_ERR(env);

  std::unordered_set<Tag*>                            all_set; // superset of all other sets
  std::unordered_set<Tag*>                            direct;  // direclty tagged on the entity
  std::unordered_map<Tag*, Tag*>                      parents; // parents of other tags
  std::unordered_map<Tag*, std::unordered_set<Tag*> > implied; // implied by another tag on the post

  // initialize direct tags and their parents
  for(Tag *tag : entity->tags) {
    direct.insert(tag);
    all_set.insert(tag);
  }

  do {
    std::unordered_set<Tag*> tmp_all_set = all_set;

    for(Tag *tag : all_set) {
      auto p = tag;
      while(p->parent) {
        tmp_all_set.insert(p->parent);
        parents.insert(std::make_pair(p->parent, p));
        p = p->parent;
      }

      // walk impliers
      for(Tag *imp : tag->implies) {
        auto i = implied.find(imp);
        if(i == implied.end()) {
          implied.insert(std::make_pair(imp, std::unordered_set<Tag*>()));
          i = implied.find(imp);
        }
        assert(i != implied.end());
        tmp_all_set.insert(imp);
        (*i).second.insert(tag);
      }
    }

    // iterate over tags that are all directly on the entity
    // walk parent tree

    if(tmp_all_set.size() == all_set.size()) break;
    all_set = std::move(tmp_all_set);
  } while(true);

  ERL_NIF_TERM res_list = enif_make_list(env, 0); // start with empty list

  for(Tag* tag : direct) {
    auto term = binary_from_string(tag->value, env);
    if(enif_is_exception(env, term)) return term;
    res_list = enif_make_list_cell(env,
      enif_make_tuple2(env, enif_make_atom(env, "direct"), term), res_list);
  }

  for(auto pair : parents) {
    auto implied = pair.first;
    auto implier = pair.second;

    auto imterm = binary_from_string(implied->value, env);
    if(enif_is_exception(env, imterm)) return imterm;
    auto erterm = binary_from_string(implier->value, env);
    if(enif_is_exception(env, erterm)) return erterm;

    res_list = enif_make_list_cell(env,
      enif_make_tuple3(env, enif_make_atom(env, "parent"), imterm, erterm), res_list);
  }

  // for(auto pair : implied) {
  //   auto implied = pair.first;
  //   auto implier = pair.second;

  //   auto imterm = binary_from_string(implied->value, env);
  //   if(enif_is_exception(env, imterm)) return imterm;
  //   auto erterm = binary_from_string(implier->value, env);
  //   if(enif_is_exception(env, erterm)) return erterm;

  //   res_list = enif_make_list_cell(env,
  //     enif_make_tuple3(env, enif_make_atom(env, "implied"), imterm, erterm), res_list);
  // }

  return enif_make_tuple2(env, A_OK(env), res_list);
}

// {handle, clause}
ERL_FUNC(do_query) {
  ENSURE_ARG(argc == 2);
  ENSURE_CONTEXT(env, argv[0]);

  if(debug) std::cout << "native: do_query called" << std::endl;

  QueryClause *c = build_clause(env, argv[1], context);
  if(c == nullptr) { return A_ERR(env); }

  ERL_NIF_TERM res_list = enif_make_list(env, 0); // start with empty list

  context->query(c, [&](const Entity* e) {
    auto term = enif_make_uint(env, e->id);
    res_list = enif_make_list_cell(env, term, res_list);
  });

  delete c;

  return enif_make_tuple2(env, A_OK(env), res_list);
}


ERL_FUNC(make_tag_parent) {
  ENSURE_ARG(argc == 3);
  ENSURE_CONTEXT(env, argv[0]);

  auto parent = get_tag_from_arg(context, env, argv[1]);
  if(!parent) return A_ERR(env);

  auto child = get_tag_from_arg(context, env, argv[2]);
  if(!child) return A_ERR(env);

  // set parent
  if(!child->set_parent(parent)) return A_ERR(env);

  return A_OK(env);
}

ERL_FUNC(get_tag_children) {
  ENSURE_ARG(argc == 2);
  ENSURE_CONTEXT(env, argv[0]);

  Tag *tag = get_tag_from_arg(context, env, argv[1]);
  if(!tag) return A_ERR(env);

  return A_OK(env);
}

static ErlNifFunc nif_funcs[] = {
  {"init_lib",         0, init_lib,         0}, // private
  {"new",              0, new_,             0},
  {"new_tag" ,         2, new_tag,          0},
  {"num_tags",         1, num_tags,         0},
  {"new_entity",       1, new_entity,       0},
  {"num_entities",     1, num_entities,     0},
  {"add_tag",          3, add_tag,          0},
  {"entity_tags",      2, entity_tags,      0},
  {"do_query",         2, do_query,         0},
  {"make_tag_parent",  3, make_tag_parent,  0},
  {"get_tag_children", 2, get_tag_children, 0}
};

ERL_NIF_INIT(Elixir.AllTheTags, nif_funcs, NULL, NULL, NULL, NULL)
