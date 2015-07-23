#include <iostream>
#include <cassert>
#include <cstring>
#include <numeric>

#include "erl_api_helpers.h"

static bool debug = false;

static ErlNifResourceType *context_type = nullptr;

static void context_resource_cleanup(ErlNifEnv *env, void *arg) {
  UNUSED(env);
  if(debug) {
    std::cerr << "native: cleaning up resource (" << arg << ")" << std::endl;
    std::cerr.flush();
  }

  assert(arg);
  ContextWrapper *cw = (ContextWrapper*) arg;
  cw->~ContextWrapper();
}

// main entrypoint that erlang talks to to perform queries on
// a tag/entity collection context
static int init_lib(ErlNifEnv* env, void** priv_data, ERL_NIF_TERM load_info) {
  UNUSED(priv_data);
  UNUSED(load_info);

  if(debug) std::cerr << "native: init native tags api" << std::endl;

  if(context_type != nullptr) {
    if(debug) std::cerr << "native: already allocated resource type; double init?" << std::endl;
    return -1;
  }

  context_type = enif_open_resource_type(env,
    nullptr, "tags_nif_handle",
    &context_resource_cleanup,
    ERL_NIF_RT_CREATE, nullptr);

  if(context_type == nullptr) {
    if(debug) std::cerr << "native: couldn't make resource type" << std::endl;
    return -2;
  }

  if(debug) std::cerr << "native: done with initialize" << std::endl;
  return 0;
}

ERL_FUNC(new_) {
  UNUSED(argv);
  ENSURE_ARG(argc == 0);

  assert(context_type);
  ContextWrapper *cw = (ContextWrapper*)enif_alloc_resource(context_type, sizeof(ContextWrapper));
  if(cw == nullptr) {
    if(debug) std::cerr << "native: could not allocate resource" << std::endl;
    return A_ERR(env);
  }
  else {
    if(debug) std::cerr << "native: allocated a resource (" << (void*)cw << ", " << sizeof(Context) << " bytes)" << std::endl;
  }

  // call ctor on erlang allocated memory
  new(cw) ContextWrapper();

  auto term = enif_make_resource(env, cw);
  enif_release_resource(cw);
  return enif_make_tuple2(env, A_OK(env), term);
}

// new_tag(handle, tag_id \\ nil) :: {:ok, tag_id}
ERL_FUNC(new_tag) {
  ENSURE_ARG(argc == 2);
  ENSURE_CONTEXT(env, argv[0]);
  WriteLock lock(cw);

  Tag *t;
  if(enif_compare(argv[1], enif_make_atom(env, "nil")) == 0) {
    t = context.new_tag();
  }
  else {
    id_type id;
    ENSURE_ARG(enif_get_uint(env, argv[1], &id));
    t = context.new_tag(id);
  }

  if(!t) {
    return A_ERR(env);
  }

  if(debug) {
    std::cerr << "native: added tag " << t->id << std::endl;
  }

  return enif_make_tuple2(env, A_OK(env), enif_make_uint(env, t->id));
}

ERL_FUNC(num_tags) {
  ENSURE_ARG(argc == 1);
  ENSURE_CONTEXT(env, argv[0]);
  ReadLock lock(cw);

  return enif_make_uint(env, context.num_tags());
}

ERL_FUNC(new_entity) {
  ENSURE_ARG(argc == 2);
  ENSURE_CONTEXT(env, argv[0]);
  WriteLock lock(cw);

  Entity *e;

  if(enif_compare(argv[1], enif_make_atom(env, "nil")) == 0) {
    e = context.new_entity();
  }
  else {
    id_type id;
    ENSURE_ARG(enif_get_uint(env, argv[1], &id));
    e = context.new_entity(id);
  }

  return enif_make_tuple2(env, A_OK(env), enif_make_uint(env, e->id));
}

ERL_FUNC(num_entities) {
  ENSURE_ARG(argc == 1);
  ENSURE_CONTEXT(env, argv[0]);
  ReadLock lock(cw);

  return enif_make_uint(env, context.num_entities());
}

// {handle, entity_id, tag_id}
ERL_FUNC(add_tag) {
  ENSURE_ARG(argc == 3);
  ENSURE_CONTEXT(env, argv[0]);
  WriteLock lock(cw);

  id_type entity_id;
  ENSURE_ARG(enif_get_uint(env, argv[1], &entity_id));

  id_type tag_id;
  ENSURE_ARG(enif_get_uint(env, argv[2], &tag_id));

  // look up tag based on id given
  auto tag = context.tag_by_id(tag_id);
  if(!tag) return A_ERR(env);

  auto entity = context.entity_by_id(entity_id);
  if(!entity) return A_ERR(env);

  if(!entity->add_tag(tag)) return A_ERR(env);

  return A_OK(env);
}

// {handle, entity_id, tag_id}
ERL_FUNC(remove_tag) {
  ENSURE_ARG(argc == 3);
  ENSURE_CONTEXT(env, argv[0]);
  WriteLock lock(cw);

  id_type entity_id;
  ENSURE_ARG(enif_get_uint(env, argv[1], &entity_id));

  id_type tag_id;
  ENSURE_ARG(enif_get_uint(env, argv[2], &tag_id));

  // look up tag
  auto tag = context.tag_by_id(tag_id);
  if(!tag) return A_ERR(env);

  auto entity = context.entity_by_id(entity_id);
  if(!entity) return A_ERR(env);

  if(!entity->remove_tag(tag)) return A_ERR(env);

  return A_OK(env);
}

ERL_FUNC(entity_tags) {
  ENSURE_ARG(argc == 2);
  ENSURE_CONTEXT(env, argv[0]);
  ReadLock lock(cw);

  id_type entity_id;
  ENSURE_ARG(enif_get_uint(env, argv[1], &entity_id));
  auto entity = context.entity_by_id(entity_id);
  if(!entity) return A_ERR(env);

  std::unordered_set<Tag*>                            all_set; // superset of all other sets
  std::unordered_set<Tag*>                            direct;  // direclty tagged on the entity
  std::unordered_map<Tag*, std::unordered_set<Tag*> > implied; // implied by another tag on the post

  // initialize direct tags and their parents
  for(Tag *tag : entity->tags) {
    direct.insert(tag);
    all_set.insert(tag);
  }

  do {
    std::unordered_set<Tag*> tmp_all_set = all_set;

    for(Tag *tag : all_set) {
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

    if(tmp_all_set.size() == all_set.size()) break;
    all_set = std::move(tmp_all_set);
  } while(true);

  ERL_NIF_TERM res_list = enif_make_list(env, 0); // start with empty list

  auto d_atom = enif_make_atom(env, "direct");
  auto i_atom = enif_make_atom(env, "implied");

  for(Tag* tag : direct) {
    auto term = enif_make_int(env, tag->id);
    if(enif_is_exception(env, term)) return term;
    res_list = enif_make_list_cell(env,
      enif_make_tuple2(env, d_atom, term), res_list);
  }

  for(auto pair : implied) {
    auto tag      = pair.first;
    auto impliers = pair.second;

    auto impliers_term = enif_make_list(env, 0);
    for(auto t : impliers) {
      impliers_term = enif_make_list_cell(env,
        enif_make_int(env, t->id), impliers_term);
    }

    auto term = enif_make_int(env, tag->id);
    res_list = enif_make_list_cell(env,
      enif_make_tuple3(env, i_atom, term, impliers_term), res_list);
  }

  return enif_make_tuple2(env, A_OK(env), res_list);
}

// {handle, clause}
ERL_FUNC(do_query) {
  ENSURE_ARG(argc == 2);
  ENSURE_CONTEXT(env, argv[0]);

  Lmake_clean:
  while(context.is_dirty()) {
    WriteLock wlock(cw);
    context.make_clean();
  }

  ReadLock rlock(cw);
  if(context.is_dirty()) {
    // 'goto' will call the dtor on rlock, as it's jumping
    // before its decl. spec section 6.6, paragraph 2
    goto Lmake_clean;
  }

  if(debug) std::cerr << "native: do_query called" << std::endl;

  QueryClause *c = build_clause(env, argv[1], context);
  if(c == nullptr) { return A_ERR(env); }

  ERL_NIF_TERM res_list = enif_make_list(env, 0); // start with empty list

  context.query(c, [&](const Entity* e) {
    auto term = enif_make_uint(env, e->id);
    res_list = enif_make_list_cell(env, term, res_list);
  });

  delete c;

  return enif_make_tuple2(env, A_OK(env), res_list);
}

ERL_FUNC(imply_tag) {
  ENSURE_ARG(argc == 3);
  ENSURE_CONTEXT(env, argv[0]);
  WriteLock lock(cw);

  Tag *implier = get_tag_from_arg(context, env, argv[1]);
  if(!implier) return A_ERR(env);
  Tag *implied = get_tag_from_arg(context, env, argv[2]);
  if(!implied) return A_ERR(env);

  if(!implier->imply(implied)) {
    return A_ERR(env);
  }

  return A_OK(env);
}
ERL_FUNC(unimply_tag) {
  ENSURE_ARG(argc == 3);
  ENSURE_CONTEXT(env, argv[0]);
  WriteLock lock(cw);

  Tag *implier = get_tag_from_arg(context, env, argv[1]);
  if(!implier) return A_ERR(env);
  Tag *implied = get_tag_from_arg(context, env, argv[2]);
  if(!implied) return A_ERR(env);

  if(!implier->unimply(implied)) {
    return A_ERR(env);
  }

  return A_OK(env);
}

ERL_FUNC(get_implies) {
  ENSURE_ARG(argc == 2);
  ENSURE_CONTEXT(env, argv[0]);
  ReadLock lock(cw);

  Tag *tag = get_tag_from_arg(context, env, argv[1]);
  if(!tag) return A_ERR(env);

  ERL_NIF_TERM res_list = enif_make_list(env, 0); // start with empty list

  for(auto i : tag->implies) {
    res_list = enif_make_list_cell(env, enif_make_int(env, i->id), res_list);
  }

  return enif_make_tuple2(env, A_OK(env), res_list);
}
ERL_FUNC(get_implied_by) {
  ENSURE_ARG(argc == 2);
  ENSURE_CONTEXT(env, argv[0]);
  ReadLock lock(cw);

  Tag *tag = get_tag_from_arg(context, env, argv[1]);
  if(!tag) return A_ERR(env);

  ERL_NIF_TERM res_list = enif_make_list(env, 0); // start with empty list

  for(auto i : tag->implied_by) {
    res_list = enif_make_list_cell(env, enif_make_int(env, i->id), res_list);
  }

  return enif_make_tuple2(env, A_OK(env), res_list);
}

ERL_FUNC(is_dirty) {
  ENSURE_ARG(argc == 1);
  ENSURE_CONTEXT(env, argv[0]);
  ReadLock lock(cw);

  return context.is_dirty() ? enif_make_atom(env, "true") : enif_make_atom(env, "false");
}

ERL_FUNC(mark_dirty) {
  ENSURE_ARG(argc == 1);
  ENSURE_CONTEXT(env, argv[0]);
  WriteLock lock(cw);

  context.mark_dirty();
  return A_OK(env);
}
#if 0
ERL_FUNC(make_clean) {
  ENSURE_ARG(argc == 1);
  ENSURE_CONTEXT(env, argv[0]);
  WriteLock lock(cw);

  context.make_clean();

  return A_OK(env);
}
#endif

static ErlNifFunc nif_funcs[] = {
  {"new",              0, new_,             0},
  {"new_tag" ,         2, new_tag,          0},
  {"num_tags",         1, num_tags,         0},
  {"new_entity",       2, new_entity,       0},
  {"num_entities",     1, num_entities,     0},
  {"add_tag",          3, add_tag,          0},
  {"remove_tag",       3, remove_tag,       0},
  {"entity_tags",      2, entity_tags,      0},
  {"do_query",         2, do_query,         0},
  {"imply_tag",        3, imply_tag,        0},
  {"unimply_tag",      3, unimply_tag,      0},
  {"get_implies",      2, get_implies,      0},
  {"get_implied_by",   2, get_implied_by,   0},
  {"is_dirty",         1, is_dirty,         0},
  {"mark_dirty",       1, mark_dirty,       0}
  // {"make_clean",       1, make_clean,       0}
};

ERL_NIF_INIT(Elixir.AllTheTags, nif_funcs, init_lib, NULL, NULL, NULL)
