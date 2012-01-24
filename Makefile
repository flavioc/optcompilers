
CXXFLAGS = $(shell $(HOME)/llvm/bin/llvm-config --cxxflags all) -g -O0

all: FunctionInfo.so

FunctionInfo.so: FunctionInfo.o
	g++ -dylib -flat_namespace -undefined suppress FunctionInfo.o -o FunctionInfo.so

%.so: %.o
	$(CXX) $(CXXFLAGS) -shared -fPIC $^ -o $@

clean:
	rm -f FunctionInfo.o *~ *.so

load:
	opt -load FunctionInfo.so -function-info loop.o -o out

LLVM = llvm-gcc -O -emit-llvm -c

compile:
	$(LLVM) other.c
