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

  test "case of tag does not matter", %{handle: handle} do
    assert :ok == AllTheTags.new_tag(handle, "foo")
    assert :error == AllTheTags.new_tag(handle, "Foo")
    assert :error == AllTheTags.new_tag(handle, "FOO")
  end

  test "case doesn't matter (2)", %{handle: handle} do
    assert :ok == AllTheTags.new_tag(handle, "Foo")
    assert :error == AllTheTags.new_tag(handle, "foo")
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
end
