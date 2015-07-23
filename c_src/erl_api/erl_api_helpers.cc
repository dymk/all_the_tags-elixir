#include <string>
#include <cstring>

#include "erl_api_helpers.h"

// converts erlang clause AST into native AST representation
QueryClause *build_clause(ErlNifEnv *env, const ERL_NIF_TERM term, const Context& c) {
  if(enif_is_number(env, term)) {

    auto tag = get_tag_from_arg(c, env, term);
    if(!tag) return nullptr;

    return build_lit(tag);
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

      return build_not(e);
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

      return matches_and ? build_and(l, r) : build_or(l, r);
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

Tag *get_tag_from_arg(const Context& context, ErlNifEnv* env, ERL_NIF_TERM term) {

  id_type tag_id;
  if(!enif_get_uint(env, term, &tag_id)) {
    return nullptr;
  }

  return context.tag_by_id(tag_id);
}
