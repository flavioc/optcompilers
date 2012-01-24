
CXXFLAGS = $(shell $(HOME)/llvm/bin/llvm-config --cxxflags all) -g -O0

all: FunctionInfo.so

FunctionInfo.so: FunctionInfo.o
	g++ -dylib -flat_namespace -undefined suppress FunctionInfo.o -o FunctionInfo.so

%.so: %.o
	$(CXX) $(CXXFLAGS) -shared -fPIC $^ -o $@

clean:
	rm -f *.o *~ *.so *.finfo *.out

TEST = opt -load FunctionInfo.so -function-info

load:
	$(TEST) loop.o -o loop.out
	$(TEST) other.o -o other.out

LLVM = llvm-gcc -O -emit-llvm -c

compile:
	$(LLVM) other.c
	$(LLVM) loop.c
