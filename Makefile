ERLANG_PATH = $(shell erl -eval 'io:format("~s", [lists:concat([code:root_dir(), "/erts-", erlang:system_info(version), "/include"])])' -s init stop -noshell)
CFLAGS = -g -std=c++11 -pedantic -Wall -Wextra -fPIC -I$(ERLANG_PATH) -Ic_src
LDFLAGS += -dynamiclib -undefined dynamic_lookup

SOURCE_FILES = $(wildcard c_src/*.cc)
H_FILES = $(wildcard c_src/*.h)

priv_dir/lib_tags.so: priv_dir $(SOURCE_FILES) $(H_FILES)
	$(CXX) -o $@ $(SOURCE_FILES) $(LDLIBS) $(LDFLAGS) $(CFLAGS) -shared

priv_dir:
	mkdir -p priv_dir

clean:
	rm -rf priv_dir/*
