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

  def num_tags(_handle) do
    not_loaded
  end

  defp init_lib() do
    not_loaded
  end

  defp not_loaded do
    "NIF library not loaded"
  end
end
