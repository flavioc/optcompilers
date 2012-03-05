
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
    typedef std::vector<std::string> var_names;

    var_vector vars;
    var_names names;

    llvm::Function *fun;

    void giveInstructionsNames(llvm::Function&);
    void buildVarsVector(llvm::Function&);
    void addVariable(const std::string&);

public:

    typedef std::vector<bool> bitvector; // XXX duplicate from IterativeFramework

    bitvector getUniversalSet(void) const;
    bitvector getEmptySet(void) const;
    bitvector getFlagedFunctionArgs(void) const;

    size_t getVarIndex(const std::string&) const;
    std::string getVarName(const size_t) const;
    void printVars(void) const;

    explicit Variables(llvm::Function&);

    ~Variables(void) {}
};


#endif
