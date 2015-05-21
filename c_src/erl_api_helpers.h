#ifndef __ERL_HELPERS_H__
#define __ERL_HELPERS_H__

#include <cassert>

#include "erl_nif.h"
#include "query.h"
#include "context.h"

#define UNUSED(x) (void)(x);
#define ENSURE_ARG(get) do { if(!(get)) { return enif_make_badarg(env); }} while(0);

#define A_OK(env) enif_make_atom(env, "ok")
#define A_ERR(env) enif_make_atom(env, "error")

#define ERL_FUNC(func_name) static ERL_NIF_TERM func_name(ErlNifEnv *env, int argc, const ERL_NIF_TERM *argv)

#define xstr(s) str(s)
#define str(s) #s

#define ENSURE_CONTEXT(env, arg) \
  Context *context = nullptr; \
  assert(context_type); \
  ENSURE_ARG(enif_get_resource(env, arg, context_type, (void**)&context)); \
  assert(context);

// converts a term query clause into its C++ AST representation
// caller is responsible for deleteing the returned QueryClause
QueryClause *build_clause(ErlNifEnv *env, const ERL_NIF_TERM term, const Context* c);

// convert a binary or list to a C string
// returns the lenght of the C string, or <=0 if there was an error (not a list/binary, buf too small)
int enif_binary_or_list_to_string(ErlNifEnv *env, ERL_NIF_TERM term, char *buf, unsigned int buflen);

// Returns the Tag (or nullptr) that corresponds to the binary/list in the given context
Tag *get_tag_from_arg(Context*, ErlNifEnv *env, ERL_NIF_TERM);

ERL_NIF_TERM binary_from_string(const std::string& str, ErlNifEnv *env);

#endif /* __ERL_HELPERS_H__ */
