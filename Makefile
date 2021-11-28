CXXFLAGS = -Wall -Wextra -std=c++20

ifeq ($(OS),Windows_NT)
	CXX = clang++
	OUT = lst.exe
	LDFLAGS = -lAdvapi32 -lOle32
	ifeq ($(DEBUG), 1)
		CXXFLAGS += -O0 -g -gcodeview
		LDFLAGS += -O0 -g -gcodeview
	endif
else
	CXX = g++
	OUT = lst
	CXXFLAGS += -Wexpansion-to-defined
	ifeq ($(DEBUG), 1)
		CXXFLAGS += -O0 -g
		LDFLAGS += -O0 -g
	endif
endif

ifndef $(DEBUG)
	CXXFLAGS += -O3 -march=native -mtune=native
endif

SRC = natural_sort.cc match.cc columns.cc unicode.cc args.cc lst.cc main.cc
OBJ = $(patsubst %.cc,build/%.o,$(SRC))
OBJ += build/arena_alloc.o
DEP = $(wildcard source/*.hh)

all: source/stdafx.hh.gch $(OUT)

source/stdafx.hh.gch: source/stdafx.hh
	@echo ' CXX  $@'
	@$(CXX) $(CXXFLAGS) -o $@ $<

build/%.o: source/%.cc $(DEP)
	@echo ' CXX  $@'
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

build/arena_alloc.o: source/arena_alloc/arena_alloc.cc source/arena_alloc/arena_alloc.hh
	@echo ' CXX  $@'
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUT): $(OBJ)
	@echo ' LINK $@'
	@$(CXX) $(LDFLAGS) -o $@ $^

callgrind: $(OUT)
	valgrind --tool=callgrind --dump-instr=yes --trace-jump=yes ./$(OUT) -l

cachegrind: $(OUT)
	valgrind --tool=cachegrind --branch-sim=yes ./$(OUT) -l

clean:
	rm -f $(OBJ) $(OUT) source/stdafx.hh.gch lst.ilk lst.pdb

.PHONY: all callgrind cachegrind clean
