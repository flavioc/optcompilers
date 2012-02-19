
#ifndef VARIABLES_HPP
#define VARIABLES_HPP

#include "llvm/BasicBlock.h"
#include "llvm/Function.h"

#include <map>
#include <vector>
#include <string>

class Variables
{
private:
   
   typedef std::map<std::string, size_t> var_vector;
   
   var_vector vars;
   
   void giveInstructionsNames(llvm::Function&);
   void buildVarsVector(llvm::Function&);
   
public:
   
   typedef std::vector<bool> bitvector; // XXX duplicate from IterativeFramework
   
   bitvector getUniversalSet(void) const;
   bitvector getEmptySet(void) const;
   size_t getVarIndex(const std::string&) const;
   void printVars(void) const;
   
   explicit Variables(llvm::Function&);
   
   ~Variables(void) {}
};


#endif
