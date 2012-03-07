
#ifndef BLOCKVARIABLES_HPP
#define BLOCKVARIABLES_HPP

#include "Variables.hpp"

#include "llvm/BasicBlock.h"
#include "llvm/Function.h"

#include <map>
#include <vector>
#include <string>

class BlockVariables :  public Variables{
public:

//    typedef std::vector<bool> bitvector; // XXX duplicate from IterativeFramework

    virtual void printVars(void) const;

    explicit BlockVariables(llvm::Function&);

    ~BlockVariables(void) {}


protected:

    typedef std::map<std::string, size_t> var_vector;
    typedef std::vector<std::string> var_names;

//    var_vector vars;
//    var_names names;

//    llvm::Function *fun;

    virtual void giveInstructionsNames(llvm::Function&);
    virtual void buildVarsVector(llvm::Function&);

};


#endif
