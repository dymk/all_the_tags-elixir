defmodule AllTheTags do
  @opaque t :: __MODULE__

  @on_load :init
  @spec init :: :ok
  def init() do
    path = :filename.join(:code.priv_dir(:all_the_tags), 'lib_tags')
    :erlang.load_nif(path, 0)
    :ok
  end

  @spec new :: t
  def new() do
    not_loaded
  end

  def new_tag(_handle, _tag_id \\ nil) do
    not_loaded
  end

  def new_entity(_handle, _entity_id \\ nil) do
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

  def remove_tag(_handle, _entity, _tag) do
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

  def mark_dirty(_handle), do: not_loaded
  def is_dirty(_handle),   do: not_loaded

  defp not_loaded do
    "NIF library not loaded"
  end
end
