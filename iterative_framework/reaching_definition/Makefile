CXXFLAGS = -rdynamic $(shell $(HOME)/WORKING/builds/current/llvm/bin/llvm-config --cxxflags all) -g
#CXXFLAGS = $(shell $(HOME)/llvm/bin/llvm-config --cxxflags all) -g -O0


all: ReachingDef.so

#ReachingDef.so: ReachingDef.o
#	g++ -dylib -flat_namespace -undefined suppress ReachingDef.o -o ReachingDef.so
#	g++ -dylib -flat_namespace -undefined  ReachingDef.o -o ReachingDef.so

%.so: %.o
	$(CXX) $(CXXFLAGS) -shared -fPIC $^ -o $@

clean:
	rm -f *.o *~ *.so *.finfo *.out *.ll

TEST = opt -load ./ReachingDef.so -reach

load: compile all
	$(TEST) loop.o -o loop.out
	$(TEST) other.o -o other.out

#LLVM = llvm-gcc -O -emit-llvm -c
LLVM = clang -O -emit-llvm -c

compile:
	$(LLVM) other.c
	$(LLVM) loop.c


dis: compile
	llvm-dis other.o
	llvm-dis loop.o
	cat other.o.ll
	cat loop.o.ll
