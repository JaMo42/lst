EXE = lst.exe

CXX = clang++
CXX_FLAGS = -std=c++17 -O2 -march=native
WIN_LIBS = -luser32 -lkernel32 -ladvapi32

SOURCES = $(wildcard Source/*.cpp)

all:
	$(CXX) $(CXX_FLAGS) $(WIN_LIBS) -o $(EXE) $(SOURCES)
