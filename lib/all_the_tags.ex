defmodule AllTheTags do
  @on_load :init
  def init() do
    :erlang.load_nif("./priv_dir/lib_tags", 0)
    :ok = init_lib
    :ok
  end

  def new() do
    not_loaded
  end

  def new_tag(_handle, _tag_val) do
    not_loaded
  end
  def new_entity(_handle) do
    not_loaded
  end

  def num_tags(_handle) do
    not_loaded
  end
  def num_entities(_handle) do
    not_loaded
  end

  def add_tag(_handle, _entity, _tag) do
    not_loaded
  end

  def entity_tags(_handle, _entity) do
    not_loaded
  end

  def do_query(_handle, _q) do
    not_loaded
  end

  def imply_tag(_handle, _implier, _implied),   do: not_loaded
  def unimply_tag(_handle, _implier, _implied), do: not_loaded
  def get_implies(_handle, _tag), do: not_loaded
  def get_implied_by(_handle, _tag), do: not_loaded

  defp init_lib() do
    not_loaded
  end

  defp not_loaded do
    "NIF library not loaded"
  end
end
