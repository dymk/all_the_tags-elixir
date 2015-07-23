defmodule AllTheTagsTest do
  use ExUnit.Case

  setup do
    {:ok, handle} = AllTheTags.new
    {:ok, handle: handle}
  end

  test "handle can be allocated" do
    assert {:ok, _handle} = AllTheTags.new
  end

  test "handle can have tags added to it", %{handle: handle} do
    assert :ok == AllTheTags.new_tag(handle, "foo")
    assert :error == AllTheTags.new_tag(handle, "foo")
  end

  test "case of tag matters", %{handle: handle} do
    assert :ok    == AllTheTags.new_tag(handle, "foo")
    assert :ok    == AllTheTags.new_tag(handle, "Foo")
    assert :ok    == AllTheTags.new_tag(handle, "FOO")
    assert :error == AllTheTags.new_tag(handle, "FOO")
  end

  test "num_tags works", %{handle: handle} do
    assert 0 == AllTheTags.num_tags(handle)
    AllTheTags.new_tag(handle, "foo")
    assert 1 == AllTheTags.num_tags(handle)
    AllTheTags.new_tag(handle, "foo")
    assert 1 == AllTheTags.num_tags(handle)
    AllTheTags.new_tag(handle, "bar")
    assert 2 == AllTheTags.num_tags(handle)
  end

  test "can create new entities", %{handle: handle} do
    refute AllTheTags.new_entity(handle) == nil
    refute AllTheTags.new_entity(handle) == nil
    refute AllTheTags.new_entity(handle) == nil
  end

  test "num entities is valid", %{handle: handle} do
    0..5 |> Enum.each(fn(i) ->
      assert AllTheTags.num_entities(handle) == i
      refute AllTheTags.new_entity(handle) == nil
      assert AllTheTags.num_entities(handle) == i+1
    end)
  end

  test "tags can be added to an entity", %{handle: handle} do
    e = set_up_e(handle)

    assert :ok    == AllTheTags.add_tag(handle, e, "foo")
    assert :error == AllTheTags.add_tag(handle, e, "foo")
  end

  test "can retrieve a list of tags on the entity", %{handle: handle} do
    e = set_up_e(handle)

    # intially empty
    {:ok, t} = AllTheTags.entity_tags(handle, e)
    assert t == []

    # one tag
    :ok = AllTheTags.add_tag(handle, e, "foo")
    {:ok, t} = AllTheTags.entity_tags(handle, e)
    assert t == [{:direct, "foo"}]

    # two tags (no guarentee on orders)
    :ok = AllTheTags.add_tag(handle, e, "bar")
    {:ok, t} = AllTheTags.entity_tags(handle, e)
    assert same_lists(t, [{:direct, "foo"}, {:direct, "bar"}])
  end

  test "deep implied tags are retrieved", %{handle: handle} do
    ["a", "b", "c", "d", "e"] |> Enum.each(&(AllTheTags.new_tag(handle, &1)))

    handle |> AllTheTags.imply_tag("a", "b")
    handle |> AllTheTags.imply_tag("a", "c")
    handle |> AllTheTags.imply_tag("c", "d")
    handle |> AllTheTags.imply_tag("b", "d")
    handle |> AllTheTags.imply_tag("d", "e")

    #     a
    #    / \
    #   b   c
    #    \ /
    #     d -> e

    e = set_up_e(handle)
    handle |> AllTheTags.add_tag(e, "a")

    {:ok, tags_list} = AllTheTags.entity_tags(handle, e)
    tags_list = implies_set_to_hash(tags_list)

    should_be = [
      {:direct, "a"},
      {:implied, "b", ["a"]},
      {:implied, "c", ["a"]},
      {:implied, "d", ["b", "c"]},
      {:implied, "e", ["d"]}
    ] |> implies_set_to_hash

    assert HashSet.equal?(tags_list, should_be)

    ["a", "b", "c", "d", "e"] |> Enum.map(fn(tag) ->
      assert {:ok, [e]} == AllTheTags.do_query(handle, tag)
    end)
  end

  test "tag query works", %{handle: handle} do
    e = set_up_e(handle)

    # empty query matches all tags
    res = handle |> AllTheTags.do_query(nil)
    assert res == {:ok, [e]}

    ["foo", "bar", {:and, "foo", "bar"}, {:or, "foo", "bar"}] |> Enum.each(fn(q) ->
      res = handle |> AllTheTags.do_query(q)
      assert res == {:ok, []}
    end)
  end

  test "query on entity with tags", %{handle: handle} do
    e = set_up_e(handle)
    f = handle |> AllTheTags.new_entity
    g = handle |> AllTheTags.new_entity

    :ok = handle |> AllTheTags.add_tag(e, "foo")
    :ok = handle |> AllTheTags.add_tag(f, "bar")
    # e has "foo"
    # f has "bar"
    # g has none

    {:ok, res} = handle |> AllTheTags.do_query("foo")
    assert same_lists(res, [e])

    {:ok, res} = handle |> AllTheTags.do_query("bar")
    assert same_lists(res, [f])

    {:ok, res} = handle |> AllTheTags.do_query({:or, "foo", "bar"})
    assert same_lists(res, [e, f])

    {:ok, res} = handle |> AllTheTags.do_query({:and, "foo", "bar"})
    assert same_lists(res, [])

    # nil should include all entities
    {:ok, res} = handle |> AllTheTags.do_query(nil)
    assert same_lists(res, [e, f, g])

    {:ok, res} = handle |> AllTheTags.do_query({:not, nil})
    assert same_lists(res, [])

    {:ok, res} = handle |> AllTheTags.do_query({:not, "foo"})
    assert same_lists(res, [f, g])
  end

  test "query with unknown tags", %{handle: handle} do
    e = handle |> AllTheTags.new_entity
    assert {:ok, [e]} == AllTheTags.do_query(handle, nil)
    assert :error     == AllTheTags.do_query(handle, "blah")
  end

  test "get_implies works", %{handle: handle} do
    handle |> AllTheTags.new_tag("foo")
    handle |> AllTheTags.new_tag("bar")
    handle |> AllTheTags.new_tag("baz")

    assert AllTheTags.get_implies(handle, "foo") == {:ok, []}
    AllTheTags.imply_tag(handle, "foo", "bar")
    assert AllTheTags.get_implies(handle, "foo") == {:ok, ["bar"]}
    AllTheTags.imply_tag(handle, "foo", "baz")
    assert AllTheTags.get_implies(handle, "foo") == {:ok, ["bar", "baz"]}
  end

  test "get_implied_by works", %{handle: handle} do
    handle |> AllTheTags.new_tag("foo")
    handle |> AllTheTags.new_tag("bar")
    handle |> AllTheTags.new_tag("baz")

    assert AllTheTags.get_implied_by(handle, "bar") == {:ok, []}
    AllTheTags.imply_tag(handle, "foo", "bar")
    assert AllTheTags.get_implied_by(handle, "bar") == {:ok, ["foo"]}
    AllTheTags.imply_tag(handle, "baz", "bar")
    assert AllTheTags.get_implied_by(handle, "bar") == {:ok, ["foo", "baz"]}
  end

  test "inserting entities in parallel works", %{handle: handle} do
    assert 0 == AllTheTags.num_entities(handle)

    num = 100
    per = 1000

    procs = Enum.map(1..num, fn(_) ->
      Task.async fn ->
        1..per |> Enum.each(fn(_) ->
          handle |> AllTheTags.new_entity
        end)
      end
    end)

    procs |> Enum.each(&Task.await(&1))

    assert num*per == AllTheTags.num_entities(handle)
  end

  test "can dirty the context, and still call query on it", %{handle: handle} do
    e = handle |> set_up_e

    handle |> AllTheTags.add_tag(e, "foo")

    handle |> AllTheTags.imply_tag("foo", "bar")
    handle |> AllTheTags.imply_tag("bar", "foo")
    assert false == handle |> AllTheTags.is_dirty
    assert {:ok, [e]} == handle |> AllTheTags.do_query("foo")
    assert {:ok, [e]} == handle |> AllTheTags.do_query("bar")

    handle |> AllTheTags.unimply_tag("foo", "bar")
    assert true == handle |> AllTheTags.is_dirty

    assert {:ok, [e]} == handle |> AllTheTags.do_query("foo")
    assert {:ok, []}  == handle |> AllTheTags.do_query("bar")

    assert false == handle |> AllTheTags.is_dirty
  end

  test "can remove tags", %{handle: handle} do
    e = handle |> set_up_e

    assert :ok        == AllTheTags.add_tag(handle, e, "foo")
    assert {:ok, [e]} == AllTheTags.do_query(handle, "foo")

    assert :ok       == AllTheTags.remove_tag(handle, e, "foo")
    assert {:ok, []} == AllTheTags.do_query(handle, "foo")

    assert :error == AllTheTags.remove_tag(handle, e, "foo")
  end

  test "can create tags with specific IDs", %{handle: handle} do
    t1 = handle |> AllTheTags.new_tag("foo", 1)
    assert t1 == :ok
  end

  defp set_up_e(handle) do
    handle |> AllTheTags.new_tag("foo")
    handle |> AllTheTags.new_tag("bar")
    e = handle |> AllTheTags.new_entity
    e
  end
  defp same_lists(l1, l2) do
    HashSet.equal? list_to_hs(l1), list_to_hs(l2)
  end
  defp list_to_hs(list) do
    Enum.into(list, HashSet.new)
  end

  defp implies_set_to_hash(list) do
    list
    |> Enum.map(fn
      {:implied, tag, impliers} -> {:implied, tag, list_to_hs(impliers)}
      other -> other
    end)
    |> list_to_hs
  end
end
