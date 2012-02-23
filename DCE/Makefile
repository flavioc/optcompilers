
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
	rm -f *.o *~ *.so *.finfo *.out *.opt.o *.opt *.s *.ll

TEST = opt -load $(FILE).so -dce-pass

clean-examples:
	rm -f *.opt *.unopt *.opt.o *.unopt.o

load: cast.opt cast.unopt
#unreachable.opt unreachable.unopt
#switch.opt switch.unopt
#faint.opt faint.unopt if.opt if.unopt simple.opt simple.unopt \
#for-fi.opt for-fi.unopt

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
