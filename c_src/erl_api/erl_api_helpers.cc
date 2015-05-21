#include <string>
#include <cstring>

#include "erl_api_helpers.h"

// converts erlang clause AST into native AST representation
QueryClause *build_clause(ErlNifEnv *env, const ERL_NIF_TERM term, const Context* c) {
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

Tag *get_tag_from_arg(Context* context, ErlNifEnv* env, ERL_NIF_TERM term) {
  char tag_val[100];
  if(enif_binary_or_list_to_string(env, term, tag_val, 100) <= 0) {
    return nullptr;
  }

  std::string stag_val(tag_val);

  // look up tag based on value given
  return context->tag_by_value(stag_val);
}

ERL_NIF_TERM binary_from_string(const std::string& str, ErlNifEnv *env) {
  ErlNifBinary bin;
  if(!enif_alloc_binary(str.size(), &bin)) return A_ERR(env);
  // copy c string value into the binary
  assert(bin.size == str.size());
  strncpy((char*)bin.data, str.c_str(), bin.size);

  return enif_make_binary(env, &bin);
}
