
#ifndef VARIABLES_HPP
#define VARIABLES_HPP

#include "llvm/BasicBlock.h"
#include "llvm/Function.h"

#include <map>
#include <vector>
#include <string>

class Variables
{
protected:

    typedef std::map<std::string, size_t> var_vector;
    typedef std::vector<std::string> var_names;

    var_vector vars;
    var_names names;

    llvm::Function *fun;

    virtual void giveInstructionsNames(llvm::Function&);
    virtual void buildVarsVector(llvm::Function&);
    virtual void addVariable(const std::string&);

public:

    typedef std::vector<bool> bitvector; // XXX duplicate from IterativeFramework

    virtual bitvector getUniversalSet(void) const;
    virtual bitvector getEmptySet(void) const;
    virtual bitvector getFlagedFunctionArgs(void) const;

    virtual size_t getVarIndex(const std::string&) const;
    virtual std::string getVarName(const size_t) const;
    virtual void printVars(void) const;

    Variables();
    Variables(llvm::Function&);

    ~Variables(void) {}
};


#endif
