   
#undef NDEBUG
#include "Variables.hpp"
#include "llvm/InstrTypes.h"
#include "llvm/Instructions.h"

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

string
Variables::getVarName(const size_t index) const
{
   assert(index < names.size());
   return names[index];
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
Variables::addVariable(const string& name)
{
   if(name == string(""))
      return;
      
   var_vector::iterator it(vars.find(name));
   if(it != vars.end())
      return;
      
   const size_t id(vars.size());
   
   vars[name] = id;
   names.push_back(name);
   assert(vars.size() == names.size());
   assert(vars.size() == id + 1);
   assert(names.size()-1 == vars[name]);
}

void
Variables::buildVarsVector(Function& F)
{
   for (Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB) {
      for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
         assert(I->hasName() || I->getType()->isVoidTy());
         Instruction &i(*I);
         
         if(isa<StoreInst>(i)) {
            StoreInst *s((StoreInst*)&i);
            Value *ptr(s->getPointerOperand());
            Value *val(s->getValueOperand());
            
            addVariable(ptr->getName());
            addVariable(val->getName());
         } else if(!i.getType()->isVoidTy()) {
            addVariable(i.getName());
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