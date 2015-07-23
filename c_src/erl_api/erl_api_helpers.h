#ifndef __ERL_HELPERS_H__
#define __ERL_HELPERS_H__

#include <cassert>
#include <mutex>
#include <atomic>

#include "erl_nif.h"
#include "query.h"
#include "context.h"

#define UNUSED(x) (void)(x);
#define ENSURE_ARG(get) do { if(!(get)) { return enif_make_badarg(env); }} while(0);

#define A_OK(env) enif_make_atom(env, "ok")
#define A_ERR(env) enif_make_atom(env, "error")

#define ERL_FUNC(func_name) static ERL_NIF_TERM func_name(ErlNifEnv *env, int argc, const ERL_NIF_TERM *argv)

#define ENSURE_CONTEXT(env, arg) \
  ContextWrapper *cw_p = nullptr; \
  assert(context_type); \
  ENSURE_ARG(enif_get_resource(env, arg, context_type, (void**)&cw_p)); \
  assert(cw_p); \
  ContextWrapper& cw = *cw_p; \
  Context& context   = cw.context;

struct ContextWrapper {
  Context context;

  // implements a basic readers/writer mutex
  // it'll be slow, but it'll work fine for our purposes
  // and will prefer the writer over the readers
  std::mutex mutex;
  std::atomic<int> readers;
};

struct ReadLock {
  ContextWrapper& ctx;

  ReadLock(ContextWrapper& ctx_) : ctx(ctx_) {
    // TODO: perhaps use try_lock and schedule_nif if mutex is already locked
    // or locked for more than 100ns or something
    // maybe: check 'writing' atomic bool flag if a WriteLock has locked the
    // context?
    ctx.mutex.lock();
    ctx.readers++;
    ctx.mutex.unlock();
  }

  ~ReadLock() {
    ctx.readers--;
    assert(ctx.readers >= 0);
  }
};
struct WriteLock {
  ContextWrapper& ctx;

  WriteLock(ContextWrapper& ctx_) : ctx(ctx_) {
    ctx.mutex.lock();
    // TODO: use condition variables to wake the write lock up
    // TODO: perhaps use schedule_nif here
    while(ctx.readers > 0) {}
  }

  ~WriteLock() {
    ctx.mutex.unlock();
  }
};

// converts a term query clause into its C++ AST representation
// caller is responsible for deleteing the returned QueryClause
QueryClause *build_clause(ErlNifEnv *env, const ERL_NIF_TERM term, const Context& c);

// Returns the Tag (or nullptr) that corresponds to the binary/list in the given context
Tag *get_tag_from_arg(const Context& c, ErlNifEnv *env, ERL_NIF_TERM);

#endif /* __ERL_HELPERS_H__ */
