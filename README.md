AllTheTags
==========

A CPU and memory performant tag database for Exlir/erlang, featuring fast
query times, the ability to represent complex tag hierarchies and implication
rules, query JITing and optimization, and a (comparitavely) small memory footprint.

The graph of tags and the implication rules between them can contain any number of
arbitrary cycles, allowing structures such as parent/child hierarchies, and
tag aliases, all of which remain performant when querying for.

The bulk of AllTheTags is implemented as a C++ library with no knowledge
of Elixir or Erlang, and thus can be extracted out of the Elxir project with
minimal effort. The native code is located in `c_src`, with native tests in
`c_src/tests` and Elixir facing tests (stress testing things like concurrent
access to the database) are located under `test`.

AllTheTags, when used from Elixir/erlang, is threadsafe, and database instances
can be queried simultaniously from multiple threads. The NIF API layer will
take care of any synchronization when database writes occur.

If the C++ project is used standalone, the user will have to guarentee that
reads and writes are synchronized; take a look at `erl_api.cc` as an example
of how to achieve this.

While the code is tested against a modestly sized test suite, use it at your own
risk (or even better, report bugs!), as any C++ bugs have the potential to crash
the Erlang VM or corrupt memory.


Why was most of this done in C++?
------
For starters, to learn about implementing Erlang NIFs.

Secondly, the Erlang VM is very powerful, but not great at tasks that require "bare metal"
performance. The lack of mutable state is also a fantastic feature of the language,
but means that very finely grained memory performance is difficult. Because of this,
the bulk of AllTheTags is an erlang NIF implemented in C++, which has been fairly
heavily tested under valgrind and stress tested under multithreading conditions.
The query engine uses various advanced techniques for optimizing complex queries,
such as just-in-time compilation and query tree rearrangement/deduping to get the
most speed possible.

Usage
-----

The main Elixir module exposed is `AllTheTags`:

```elixir
# Create a new instance of an AllTheTags database
{:ok, db} = AllTheTags.new

# Create some entities in the database (which are represented as integers)
# An explicit entity ID can be passed in
{:ok, e1} = AllTheTags.new_entity(db, 0)
{:ok, e2} = AllTheTags.new_entity(db)

# e1 == 0
# e2 == (some other integer)

# If the same entity is created twice, :error is returned
:error = AllTheTags.new_entity(db, 0)

# Tags are created the same way, and are represented by integer IDs
# An optional explicit ID can be passed to set the ID of the created tag
{:ok, mytag} = AllTheTags.new_tag(db, 10)

# mytag == 10

# Same as with entities, if a tag is registered with the same ID twice, :error
# will be returned
:error = AllTheTags.new_tag(db, 10)

# Create some tags in the database
@a = 0; @b = 1; @c = 2; @d = 3
[@a, @b, @c, @d] |> Enum.each(fn(tag) ->
  AllTheTags.new_tag(db, tag)
end)

# Create some implication rules between the tags
AllTheTags.imply_tag(@a, @b) # Anything tagged with 'a' will act as if it's also tagged with 'b'

# Make some more implications, to form a directed implication graph as follows (flows downwards):
#     a
#    / \
#   b   c
#    \ /
#     d
#     |
#     e
AllTheTags.imply_tag(db, @a, @c)
AllTheTags.imply_tag(db, @b, @d)
AllTheTags.imply_tag(db, @c, @d)
AllTheTags.imply_tag(db, @d, @e)

# Add the tag @b on e1, @c on e2
AllTheTags.add_tag(db, e1, @b)
AllTheTags.add_tag(db, e2, @c)

# Querying the database

AllTheTags.do_query(db, @a)
# => {:ok, []}

AllTheTags.do_query(db, @b)
# => {:ok, [e1]}

AllTheTags.do_query(db, @c)
# => {:ok, [e2]}

AllTheTags.do_query(db, @d)
# => {:ok, [e1, e2]}

AllTheTags.do_query(db, {:or, @b, @c})
# => {:ok, [e1, e2]}


# Remove implication edges with `unimply`
AllTheTags.unimply_tag(db, @a, @c)

# resulting implication graph:
#     a
#    /
#   b   c
#    \ /
#     d
#     |
#     e

```

Queries
-------

Queries are executed by calling `AllTheTags.do_query(database, <query>)`,
where query can be anything with this structure:
 - a literal integer ID of a tag
 - a tuple of `{:and, clause1, clause2}` or `{:or, clause1, clause2}`, where `clause1` and `clause2`
   are a any of these possible nodes
 - a tuple of `{:not, clause}`, which inverts the query such that matching enties *don't* match `clause`
 - `nil`, which will match all entities (the empty query)

For instance, to query for all entities which have tag `@foo = 1` and also lack tag `@bar = 2`:
 - `{:and, @foo, {:not, @bar}}`

All entities which have tag @foo or @bar or @baz:
 - `{:or, @foo, {:or, @bar, @baz}}`

Queries for all entities:
 - `nil`

Queries for none of the entities:
 - `{:not, nil}`

Other Methods
------
 - `num_tags/1` the number of tags in the database
 - `num_entities/1` the number of entities in the database
 - `get_implies/2` get all the tags that a tag implies
 - `get_implied_by/2` get all the tags that imply a given tag

Briefly, Other Methods
----
These module methods should be further documented, but here they are in brief:
 - `entity_tags/2` get the tags on an entity, both direct and implied
 - `unimply_tag/3`  remove implication edge between two tags, called like `unimply_tag(db, implier, implied)`
 - `remove_tag/3` remove tag from an entity, called like `remove_tag(db, entity, tag)`
 - `mark_dirty/1` marks the database as "dirty", it'll have to one recalculation before
 executing a query, but if marked dirty adding implication edges between tags is a cheap operation
 - `is_dirty/1` return `true` or `false` if the database is in a dirty state

For now, take a look at `all_the_tags_test.exs` for usage of the above

What is a "dirty" database?
------

Warning: implementation details follow

AllTheTags internally stores a metagraph (which is a directed acyclic graph) of all the
tag implication rules as a means to quickly test if an entity has a given tag -
or any tag implied by a tag - belong to it. The metagraph is built up
incrementally with every implication edge added (and is generally a very quick operation),
and assumes that all edges will
not be added in a single go with no query calls inbetween (such as would be the
case for a long running server only occassionally having its implication rules changed).

The algorithm for removing an edge between to tags that are mutually implicative
(i.e. belong to the same metanode), and recalculating the metagraph incrementally,
is an overly complex operation, and thus the entire metagraph is recalculated before
a query in this case. This is an `O(|V|+|E|)` (linear) operation and only needs to
happen once before all subsequent queries.

`mark_dirty/1` can be called to manually mark the database as being in a dirty state.

While the graph is dirty, the metagraph is not incrementally updated, so when
initially loading the edges into the database (such as on a full server restart),
`mark_dirty/1` should be called to speed up edge insertion.


TODO: Benchmarks
----------------

gotta add benchmarks for combinations of no query optimization,
huffman code tree optimization, JIT optimization, and combinations thereof.

