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
  int len =  enif_binary_or_list_to_string(env, argv[1], value, 100);
  if(len <= 0) {
    if(debug) std::cout << "native: value string <= 0: " << len << std::endl;
    return enif_make_badarg(env);
  }
  std::string s_value(value);

  // check if the tag already exists
  if(context->tag_by_value(s_value)) { return A_ERR(env); }

  // allocate new tag, insert into the
  context->insert_new_tag(s_value);

  if(debug) std::cout << "native: added tag with value " << s_value << std::endl;
  return A_OK(env);
}

ERL_FUNC(num_tags) {
  ENSURE_ARG(argc == 1);

  Context *context = nullptr;
  ENSURE_ARG(enif_get_resource(env, argv[0], context_type, (void**)&context));
  assert(context);

  return enif_make_int(env, context->num_tags());
}

static ErlNifFunc nif_funcs[] = {
  {"init_lib", 0, init_lib, 0}, // private
  {"new",      0, new_,     0},
  {"new_tag" , 2, new_tag,  0},
  {"num_tags", 1, num_tags, 0}
};

ERL_NIF_INIT(Elixir.AllTheTags, nif_funcs, NULL, NULL, NULL, NULL)
