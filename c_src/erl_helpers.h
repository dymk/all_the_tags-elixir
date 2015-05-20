#ifndef __ERL_HELPERS_H__
#define __ERL_HELPERS_H__

#include "erl_nif.h"
#define UNUSED(x) (void)(x);
#define ENSURE_ARG(get) do { if(!(get)) { return enif_make_badarg(env); }} while(0);

#define A_OK(env) enif_make_atom(env, "ok")
#define A_ERR(env) enif_make_atom(env, "error")

#define ERL_FUNC(func_name) static ERL_NIF_TERM func_name(ErlNifEnv *env, int argc, const ERL_NIF_TERM *argv)

#endif /* __ERL_HELPERS_H__ */
