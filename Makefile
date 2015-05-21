ERLANG_PATH = $(shell erl -eval 'io:format("~s", [lists:concat([code:root_dir(), "/erts-", erlang:system_info(version), "/include"])])' -s init stop -noshell)
CFLAGS = -g -std=c++14 -pedantic -Wall -Wextra -Ic_src
LDFLAGS += -dynamiclib -undefined dynamic_lookup

ERL_API_FILES = $(wildcard c_src/erl_api/*.cc)

SOURCE_FILES = $(wildcard c_src/*.cc)
H_FILES = $(wildcard c_src/*.h)

API_LIB_SRC = $(SOURCE_FILES) $(ERL_API_FILES)
priv_dir/lib_tags.so: priv_dir $(API_LIB_SRC) $(H_FILES) $(wildcard c_src/erl_api/*.h)
	$(CXX) -o $@ $(API_LIB_SRC) -I$(ERLANG_PATH) -fPIC $(LDLIBS) $(LDFLAGS) $(CFLAGS) -shared

LIBHAYAI = c_src/test/hayai/src/libhayai_main.a
HAYAIDIR = c_src/test/hayai/src
GTESTDIR = c_src/test/hayai/vendor/gtest/include
LIBGTEST = c_src/test/hayai/vendor/gtest/libgtest.a

TEST_FILES = $(wildcard c_src/test/*.cc)
TEST_SRC = $(SOURCE_FILES) $(TEST_FILES)
c_src/test/runner: $(LIBHAYAI) $(TEST_SRC) $(H_FILES) $(wildcard c_src/test/*.h)
	$(CXX) -o $@ $(TEST_SRC) $(LIBHAYAI) $(LIBGTEST) -I$(HAYAIDIR) -I$(GTESTDIR) $(CFLAGS)

c_src/test/hayai/src/libhayai_main.a:
	git submodule init
	git submodule update
	cd c_src/test/hayai; cmake .
	make -C c_src/test/hayai
	cd c_src/test/hayai/vendor/gtest; cmake .
	make -C c_src/test/hayai/vendor/gtest

.PHONY: test
test: c_src/test/runner
	./c_src/test/runner

priv_dir:
	mkdir -p priv_dir

clean:
	rm -rf priv_dir/*
	rm -rf c_src/test/runner
