
CXX = g++
CXXFLAGS = $(shell $(HOME)/new-llvm/bin/llvm-config --cxxflags all) -g
#CXXFLAGS = -rdynamic $(shell $(HOME)/WORKING/builds/current/llvm/bin/llvm-config --cxxflags all) -g

FILE = DCE

all: $(FILE).so

$(FILE).so: $(FILE).o IterativeFramework.o Variables.o
	g++ -dylib -undefined suppress -flat_namespace $^ -o $(FILE).so

%.so: %.o
	$(CXX) $(CXXFLAGS) -shared -fPIC $^ -o $@

clean:
	rm -f *.o *~ *.so *.finfo *.out

TEST = opt -load $(FILE).so -dce-pass

load:
	$(TEST) faint.o -o faint.out

LLVM = clang -emit-llvm -c -O0

compile:
	$(LLVM) faint.c
	$(LLVM) simple.c
