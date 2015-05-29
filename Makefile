ERLANG_PATH = $(shell erl -eval 'io:format("~s", [lists:concat([code:root_dir(), "/erts-", erlang:system_info(version), "/include"])])' -s init stop -noshell)
CFLAGS = -g -std=c++11 -Ic_src

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	LDFLAGS += -undefined dynamic_lookup -dynamiclib
endif

# when using g++, gtest requires pthreads
ifeq ($(UNAME_S),Linux)
	CFLAGS += -lpthread
endif

ERL_API_FILES = $(wildcard c_src/erl_api/*.cc)

SOURCE_FILES = $(wildcard c_src/*.cc)
H_FILES = $(wildcard c_src/*.h)

# vendored code
VENDIR   = c_src/vendor
ASMJITDIR = $(VENDIR)/asmjit
LIBASMJIT = $(VENDIR)/asmjit/libasmjit.a
CFLAGS += -I$(ASMJITDIR)/src

API_LIB_SRC = $(SOURCE_FILES) $(ERL_API_FILES) $(LIBASMJIT)
priv_dir/lib_tags.so: priv_dir $(API_LIB_SRC) $(LIBASMJIT) $(H_FILES) $(wildcard c_src/erl_api/*.h)
	$(CXX) -o $@ $(API_LIB_SRC) $(LIBASMJIT) -I$(ERLANG_PATH) -fPIC $(LDLIBS) $(LDFLAGS) $(CFLAGS) -shared

$(LIBASMJIT):
	git submodule init
	git submodule update
	cd $(ASMJITDIR); cmake -DASMJIT_STATIC:BOOL=TRUE .
	make -C $(ASMJITDIR)

TESTDIR  = c_src/test
LIBHAYAI = $(TESTDIR)/hayai/src/libhayai_main.a
HAYAIDIR = $(TESTDIR)/hayai/src
GTESTDIR = $(TESTDIR)/hayai/vendor/gtest/include
LIBGTEST = $(TESTDIR)/hayai/vendor/gtest/libgtest.a

TEST_FILES = $(wildcard $(TESTDIR)/*.cc)
TEST_SRC = $(SOURCE_FILES) $(TEST_FILES)
c_src/test/runner: $(LIBHAYAI) $(LIBASMJIT) $(TEST_SRC) $(H_FILES) $(wildcard $(TESTDIR)/*.h)
	$(CXX) -o $@ $(TEST_SRC) $(LIBASMJIT) $(LIBHAYAI) $(LIBGTEST) -I$(HAYAIDIR) -I$(GTESTDIR) -I$(TESTDIR) $(CFLAGS)

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
.PHONY: valgrind
valgrind: c_src/test/runner
	valgrind --leak-check=full ./c_src/test/runner

priv_dir:
	mkdir -p priv_dir

clean:
	rm -rf priv_dir/*
	rm -rf c_src/test/runner
