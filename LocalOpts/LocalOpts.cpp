#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Module.h"

#include <ostream>
#include <fstream>
#include <iostream>

using namespace llvm;

namespace
{

class LocalOpts : public ModulePass
{
   
   void processBlock(BasicBlock& blk)
   {
      BasicBlock::iterator it(blk.begin());
      BasicBlock::iterator end(blk.end());
      
      
      blk.dump();
   }
  
public:
	static char ID;

	LocalOpts(): ModulePass(ID)
	{
	}

	~LocalOpts()
	{
	}

  // We don't modify the program, so we preserve all analyses
  virtual void getAnalysisUsage(AnalysisUsage &AU) const
  {
     AU.setPreservesCFG();
  }
	
  virtual bool runOnModule(Module& M)
  {
    for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI)
    {
       Function& fun(*MI);

       for(Function::BasicBlockListType::iterator it(fun.begin()), end(fun.end()); it != end; ++it) {
          BasicBlock &blk(*it);
          processBlock(blk);
       }
       
    }
    return false;
  }
};

char LocalOpts::ID = 0;
RegisterPass<LocalOpts> X("local-opts", "15745: Block Optimizations");
}
