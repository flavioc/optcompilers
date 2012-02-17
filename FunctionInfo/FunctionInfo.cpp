#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"

#include <ostream>
#include <fstream>
#include <iostream>
#include <string>
#include <map>

using namespace llvm;
using namespace std;

namespace
{

class FunctionInfo : public ModulePass
{
   struct info {
      size_t args;
      size_t calls;
      size_t blocks;
      size_t insts;
   };

   typedef map<string, info> map_functions;
   map<string, info> data;

  // output the function information to a file
  void printFunctionInfo(Module& M)
  {
    std::string name = M.getModuleIdentifier() + ".finfo";
    std::ofstream file(name.c_str());

    map_functions::iterator it(data.begin());

    file << "Name\t# Args\t# Calls\t# Blocks\t# Insts" << endl;

    for(; it != data.end(); it++) {
       info& f(it->second);
       const string& name(it->first);

       file << name << '\t' << f.args << '\t' << f.calls << '\t' << f.blocks << '\t' << f.insts;

       file << endl;
    }
  }

  void findCallSites(Function &F)
  {
     // iterate over all instructions
     for(Function::BasicBlockListType::iterator bl(F.begin()); bl != F.end(); ++bl) {
        BasicBlock &block(*bl);
        BasicBlock::iterator it(block.begin());
        BasicBlock::iterator end(block.end());

        for(; it != end; ++it) {
           Instruction& i(*it);
           if(CallInst::classof(&i)) {
              CallInst *call((CallInst*)&i);
              Function *fp(call->getCalledFunction());
              string name(fp->getName().data());
              map_functions::iterator it(data.find(name));

              assert(it != data.end());

              info& f(it->second);

              f.calls++;
           }
        }
      }
  }

public:

	static char ID;

	FunctionInfo() :
	  ModulePass(ID)
	{
	}

	~FunctionInfo()
	{
	}

  // We don't modify the program, so we preserve all analyses
  virtual void getAnalysisUsage(AnalysisUsage &AU) const
  {
    AU.setPreservesAll();
  }

  virtual bool runOnFunction(Function &F)
  {
     // Function > GlobalValue > Value
     string name(F.getName().data());
     info f;

     f.args = F.getFunctionType()->getNumParams();
     f.calls = 0;

     Function::BasicBlockListType::iterator bl(F.begin());

     f.blocks = 0;
     f.insts = 0;

     for(; bl != F.end(); ++bl) {
        f.blocks++;
        f.insts += (*bl).size();
     }

     // add to dictionary
     data[name] = f;

     return false;
  }


  virtual bool runOnModule(Module& M)
  {
    for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI)
	   runOnFunction(*MI);

    for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI)
      findCallSites(*MI);

    printFunctionInfo(M);

    return false;
  }
};

char FunctionInfo::ID = 0;
RegisterPass<FunctionInfo> X("function-info", "15745: Functions Information");

}
