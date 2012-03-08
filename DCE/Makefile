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

load: global.opt \
	cast.opt \
	faint.opt faint.unopt \
	float.opt float.unopt \
	multiple.opt multiple.unopt \
	atoi.opt \
	vector.opt \
	switch.opt \
	if.opt \
	simple.opt \
	for-fi.opt

%.opt.o: %.o
	$(TEST) $^ -o $@

%.opt: %.opt.o
	llc $^ -o $@.s
	clang $@.s -o $@

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
	$(LLVM) global.c
	$(LLVM) atoi.c
	$(LLVM) for-fi.c
