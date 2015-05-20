#include <iostream>
#include <cassert>
#include <cstring>

#include "context.h"
#include "erl_helpers.h"

static bool debug = true;

int enif_binary_or_list_to_string(ErlNifEnv *env, ERL_NIF_TERM term, char *buf, unsigned int buflen) {
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
  // Context *context = (Context*) arg;
  // context->~Context();
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
  if(argc != 2) {
    if(debug) std::cout << "native: new_tag called without 2 args: " << argc << std::endl;
    return enif_make_badarg(env);
  }

  Context *context = nullptr;
  assert(context_type);
  if(!enif_get_resource(env, argv[0], context_type, (void**)&context)) {
    if(debug) std::cout << "native: could not get resource from handle" << std::endl;
    return enif_make_badarg(env);
  }
  assert(context);

  char value[100];
  ENSURE_ARG(enif_binary_or_list_to_string(env, argv[1], value, 100) > 0);
  std::string s_value(value);

  // allocate new tag, insert into the
  if(!context->new_tag(s_value)) {
    return A_ERR(env);
  }

  if(debug) std::cout << "native: added tag with value " << s_value << std::endl;
  return A_OK(env);
}

ERL_FUNC(num_tags) {
  ENSURE_ARG(argc == 1);

  Context *context = nullptr;
  ENSURE_ARG(enif_get_resource(env, argv[0], context_type, (void**)&context));
  assert(context);

  return enif_make_uint(env, context->num_tags());
}

ERL_FUNC(new_entity) {
  ENSURE_ARG(argc == 1);

  Context *context = nullptr;
  ENSURE_ARG(enif_get_resource(env, argv[0], context_type, (void**)&context));
  assert(context);

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

  Context *context = nullptr;
  ENSURE_ARG(enif_get_resource(env, argv[0], context_type, (void**)&context));
  assert(context);

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

  Context *context = nullptr;
  ENSURE_ARG(enif_get_resource(env, argv[0], context_type, (void**)&context));
  assert(context);

  id_type entity_id;
  ENSURE_ARG(enif_get_uint(env, argv[1], &entity_id));
  auto entity = context->entity_by_id(entity_id);
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

static ErlNifFunc nif_funcs[] = {
  {"init_lib",        0, init_lib,      0}, // private
  {"new",             0, new_,          0},
  {"new_tag" ,        2, new_tag,       0},
  {"num_tags",        1, num_tags,      0},
  {"new_entity",      1, new_entity,    0},
  {"num_entities",    1, num_entities,  0},
  {"add_tag",         3, add_tag,       0},
  {"entity_tags",     2, entity_tags,   0}
};

ERL_NIF_INIT(Elixir.AllTheTags, nif_funcs, NULL, NULL, NULL, NULL)
