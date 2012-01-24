
CXXFLAGS = $(shell $(HOME)/llvm/bin/llvm-config --cxxflags all) -g
FILE = LocalOpts

all: $(FILE).so

$(FILE).so: $(FILE).o
	g++ -dylib -undefined dynamic_lookup $(FILE).o -o $(FILE).so

%.so: %.o
	$(CXX) $(CXXFLAGS) -shared -fPIC $^ -o $@

clean:
	rm -f *.o *~ *.so *.finfo *.out

TEST = opt -load $(FILE).so -local-opts

load:
	$(TEST) algebraic.o -o algebraic.out
	$(TEST) constfold.o -o constfold.out
	$(TEST) strength.o -o strength.out

LLVM = llvm-gcc -emit-llvm -c

compile:
	$(LLVM) algebraic.c
	$(LLVM) constfold.c
	$(LLVM) strength.c