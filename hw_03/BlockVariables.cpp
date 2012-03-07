#undef NDEBUG
#include "Variables.hpp"
#include "BlockVariables.hpp"
#include "llvm/InstrTypes.h"
#include "llvm/Instructions.h"

#include <iostream>
#include <ostream>
#include <sstream>

using namespace llvm;
using namespace std;

BlockVariables::BlockVariables(Function& f): Variables()
{
   giveInstructionsNames(f);
   buildVarsVector(f);

   fun=&f;
}

void
BlockVariables::giveInstructionsNames(Function& F)
{
    for (Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB)
        if (!BB->hasName())
            BB->setName("bb");
}


void
BlockVariables::buildVarsVector(Function& F)
{
   for (Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB)
       addVariable(BB->getName());

}

void
BlockVariables::printVars(void) const
{
   for(var_vector::const_iterator it(vars.begin()), e(vars.end()); it != e; ++it) {
      cout << "Var: " << it->first << " " << it->second << endl;
   }
}
