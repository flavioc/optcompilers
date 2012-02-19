
#include "Variables.hpp"
#include "llvm/InstrTypes.h"

#include <iostream>

using namespace llvm;
using namespace std;

Variables::Variables(Function& f)
{
   giveInstructionsNames(f);
   buildVarsVector(f);
}

Variables::bitvector
Variables::getUniversalSet(void) const
{
   return bitvector(vars.size(), true);
}

Variables::bitvector
Variables::getEmptySet(void) const
{
   return bitvector(vars.size(), false);
}

size_t
Variables::getVarIndex(const string& name) const
{
   var_vector::const_iterator f(vars.find(name));
   
   assert(f != vars.end());
   
   return f->second;
}

void
Variables::giveInstructionsNames(Function& F)
{
   for (Function::arg_iterator AI = F.arg_begin(), AE = F.arg_end();
            AI != AE; ++AI)
      if (!AI->hasName() && !AI->getType()->isVoidTy())
         AI->setName("arg");

   for (Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB) {
      if (!BB->hasName())
         BB->setName("bb");

      for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I)
         if (!I->hasName() && !I->getType()->isVoidTy())
            I->setName("tmp");
   }
}

void
Variables::buildVarsVector(Function& F)
{
   for (Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB) {
      for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
         assert(I->hasName() || I->getType()->isVoidTy());
         
         if(!I->getType()->isVoidTy()) {
            if(I->getName() == string(""))
               I->dump();
            var_vector::iterator it(vars.find(I->getName()));
            assert(it == vars.end());
            vars[I->getName()] = vars.size();
         }
      }
   }
}

void
Variables::printVars(void) const
{
   for(var_vector::const_iterator it(vars.begin()), e(vars.end()); it != e; ++it) {
      cout << "Var: " << it->first << " " << it->second << endl;
   }
}