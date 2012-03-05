# Note: we need to include the Variables* and IterativeFramework* files.

# This variable is dependent on the system
# flavio's dir
#LLVM_DIR=$(HOME)/new-llvm
#
# Richard's dir
LLVM_DIR=$(HOME)/WORKING/builds/current/llvm


# Flavio's system
#CXXFLAGS = $(shell $(LLVM_DIR)/bin/llvm-config --cxxflags all) -g
#LIBFLAGS= -undefined suppress -dylib -flat_namespace

# Richard's System
CXXFLAGS = -rdynamic $(shell $(LLVM_DIR)/bin/llvm-config --cxxflags all) -g
#LIBFLAGS= -dylib -flat_namespace


CXX = g++

FILE = DCE

all: $(FILE).so

$(FILE).so: $(FILE).o IterativeFramework.o Variables.o
	g++ $(LIBFLAGS) $^ -o $(FILE).so

#%.so: %.o
#	$(CXX) $(CXXFLAGS) -shared -fPIC $^ -o $@

clean:
	rm -f *.o *~ *.so *.finfo *.out *.opt.o *.opt *.s *.ll

TEST = opt -load $(FILE).so -dce-pass

clean-examples:
	rm -f *.opt *.unopt *.opt.o *.unopt.o

load: cast.opt cast.unopt \
	faint.opt faint.unopt faint.llvm \
	float.opt float.unopt float.llvm \
	multiple.opt multiple.unopt multiple.llvm \
	vector.opt
#unreachable.opt unreachable.unopt
#switch.opt switch.unopt
# if.opt if.unopt \
# simple.opt simple.unopt \
#for-fi.opt for-fi.unopt

%.opt.o: %.o
	$(TEST) $^ -o $@

%.opt: %.opt.o
	llc $^ -o $@.s
	clang $@.s -o $@

%.llvm: %.o
	opt -dce $^ -o $@.o
	llc $@.o -o $@.s
	clang $@.s -o $@
	rm -f $@.o

%.unopt: %.o
	llc $^ -o $@.s
	clang $@.s -o $@

LLVM = clang -emit-llvm -c -O0

compile:
	$(LLVM) faint.c
	$(LLVM) simple.c
	$(LLVM) if.c
	$(LLVM) coiso.c
	$(LLVM) switch.c
	$(LLVM) unreachable.c
	$(LLVM) cast.c
	$(LLVM) float.c
	$(LLVM) multiple.c
	$(LLVM) vector.c
