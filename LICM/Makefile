#CXXFLAGS = -rdynamic $(shell $(HOME)/WORKING/builds/current/llvm/bin/llvm-config --cxxflags all) -g  -W
CXXFLAGS = $(shell $(HOME)/WORKING/builds/current/llvm/bin/llvm-config --cxxflags all) -g  -W
#CXXFLAGS = $(shell $(HOME)/llvm/bin/llvm-config --cxxflags all) -g -O0


all: LICM.so

#LICM.so: IterativeFramework.o LICM.o  Variables.o BlockVariables.o

LICM.so: IterativeFramework.o LICM.o  Variables.o BlockVariables.o
#	g++ -dylib -flat_namespace -undefined suppress LICM.o -o LICM.so
	g++ -shared -flat_namespace -fPIC  IterativeFramework.o LICM.o  Variables.o BlockVariables.o -o LICM.so

%.so: %.o
	$(CXX) $(CXXFLAGS) -shared -fPIC $^ -o $@

clean:
	rm -f *.o *~ *.so *.finfo *.out *.ll

#TEST = opt -load ./LICM.so -mem2reg -disable-inlining -licm-pass
TEST = opt -load ./LICM.so  -licm-pass

load: compile all
	$(TEST) loop.o -o loop.out
#	$(TEST) other.o -o other.out

#LLVM = clang -O -mem2reg -memcpyopt -O0 -emit-llvm -c
LLVM = clang -O -O0 -emit-llvm -c
#LLVM = clang -O  -O1 -emit-llvm -c

compile:
#	$(LLVM) other.c
	$(LLVM) loop.c


dis: compile
#	llvm-dis other.o
	llvm-dis loop.o
#	cat other.o.ll
	cat loop.o.ll
