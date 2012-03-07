#undef NDEBUG
#include "Variables.hpp"
#include "llvm/InstrTypes.h"
#include "llvm/Instructions.h"

#include <iostream>
#include <ostream>
#include <sstream>

using namespace llvm;
using namespace std;

Variables::Variables()
{}
Variables::Variables(Function& f)
{
   giveInstructionsNames(f);
   buildVarsVector(f);

   fun=&f;
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


// This is for reaching definitions
Variables::bitvector
Variables::getFlagedFunctionArgs(void) const
{
    const Variables *cur = this;

#define GET_NAME(VALUE) ((VALUE)->getName().str())
#define GET_INDEX(VALUE) (cur->getVarIndex(GET_NAME(VALUE)))
#define GEN_VALUE(VALUE) do { \
     cout << "Variable " << GET_NAME(VALUE) << " (" << GET_INDEX(VALUE) << ") was generated\n"; \
     out[GET_INDEX(VALUE)] = true;   \
     } while(false)

    bitvector out(vars.size(), false);


    for (Function::arg_iterator AI = fun->arg_begin(), AE = fun->arg_end();
         AI != AE; ++AI)
        if(!AI->getType()->isVoidTy())
            GEN_VALUE(AI);



    return out;

#undef GET_NAME
#undef GET_INDEX
#undef GEN_VALUE
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
            AI->setName("farg");

    for (Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB)
    {
        if (!BB->hasName())
            BB->setName("bb");

        for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I)
        {
            if (!I->hasName() && !I->getType()->isVoidTy())
                I->setName("tmp");
            else if (!I->hasName() && isa<StoreInst>(I))
            {
                StoreInst *si = (StoreInst*)&I;
                Value *ptr(si->getPointerOperand());
                Value *val(si->getValueOperand());

                if(!val->hasName())
                    val->setName("val");

                if(!ptr->hasName())
                    si->setName("store");

                assert(si != NULL);
                assert(ptr != NULL);
                //assert(false);

                //

            }

        }
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

   for (Function::arg_iterator AI = F.arg_begin(), AE = F.arg_end();
            AI != AE; ++AI)
      if (!AI->getType()->isVoidTy())
         addVariable(AI->getName());

   for (Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB) {
      for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
          assert(I->hasName() || (I->getType()->isVoidTy()/*&& !isa<StoreInst>(I)*/));
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
