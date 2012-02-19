#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/Instruction.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/LLVMContext.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "Variables.hpp"
#include "IterativeFramework.hpp"

#include <ostream>
#include <fstream>
#include <iostream>
#include <stack>
#include <map>

using namespace std;
using namespace llvm;

namespace
{

class DCE;
DCE *ptr; 
static bitvector doRunPass(CustomBlock&);  

class DCE : public ModulePass
{
public:
	static char ID;

	DCE(): ModulePass(ID)
	{
	}

	~DCE()
	{
	}

  // We don't modify the program, so we preserve all analyses
  virtual void getAnalysisUsage(AnalysisUsage &AU) const
  {
     AU.setPreservesCFG();
  }
  
  void runModule (BasicBlock &blk)
  {
     BasicBlock::iterator it(blk.begin());
     BasicBlock::iterator end(blk.end());
     
     for(; it != end; ++it) {
        Instruction& inst(*it);
        Value *vl((Value *)&inst);
        
        cout << vl->getName().str() << endl;
     }
  }
  
  bitvector runPass(CustomBlock& cblk)
  {
     // must start reading OUT
     bitvector out(cblk.out);
     
     cblk.blk->dump();
     
     return out;
  }

  virtual bool runOnModule(Module& M)
  {
     ptr = this;
     
    for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI)
    {
       Function& fun(*MI);

       Variables vars(fun);
       vars.printVars();
       
       IterativeFramework cfg(fun, IterativeFramework::DIRECTION_BACKWARD, IterativeFramework::MEET_INTERSECTION,
          doRunPass, vars.getUniversalSet());
       
       cfg.execute();
#if 0
       for(Function::BasicBlockListType::iterator it(fun.begin()), end(fun.end()); it != end; ++it) {
          BasicBlock &blk(*it);
          runModule(blk);
          blk.dump();
       }
#endif
    }
    return false;
  }
  
private:
   
   //void 
};

char DCE::ID = 0;
RegisterPass<DCE> X("dce-pass", "15745: Dead Code Elimination");

static bitvector
doRunPass(CustomBlock& cblk)
{
   return ptr->runPass(cblk);
}
}
