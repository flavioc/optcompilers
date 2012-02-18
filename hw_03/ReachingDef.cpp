#include "IterativeFramework.hpp"

#include "llvm/BasicBlock.h"
#include "llvm/Constants.h"
#include "llvm/LLVMContext.h"
#include "llvm/Pass.h"
#include "llvm/User.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include "llvm/InstrTypes.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Twine.h"

#include <ostream>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <map>

using namespace llvm;
using namespace std;

namespace
{

class ReachingDef : public ModulePass
{

public:

	static char ID;

	ReachingDef() :
        ModulePass(ID)
	{
	}

	~ReachingDef()
	{
	}

    // We don't modify the program, so we preserve all analyses
    virtual void getAnalysisUsage(AnalysisUsage &AU) const
    {
        AU.setPreservesAll();
    }

    // At the block
    // TODO: still need to think about Phi handling
    static bool defTransfer(blockPoints *bp, map<Value*,unsigned> *valuesToIndex)
    {
        vector<bool> *v_old = bp->in;

        // NOTE: Right here is where phi fuctions should be handled,
        //       between the IN  and the first point.
#if 0
        for(;;)
        {

        }
#endif

        for(map<Value*, vector<bool>*>::iterator it = bp->programPoints.begin();
            it != bp->programPoints.end(); ++it)
        {
            vector<bool> *v_temp = new vector<bool>(it->second->size());
            vector<bool> *v_temp2 = new vector<bool>(it->second->size());
            vector<bool> *v_kill = new vector<bool>(it->second->size());

            //gen
            if(isa<Instruction>(it->first)||isa<Argument>(it->first))
                (*v_temp)[(*valuesToIndex)[it->first]] = true;

            //kill: will only kill values at PHI
            if(isa<PHINode>(it->first))
            {
                PHINode *inst = (PHINode *)it->first;
                for(User::op_iterator oi= inst->op_begin(), oe= inst->op_end(); oi!=oe ;++oi)
                {
                    (*v_kill)[(*valuesToIndex)[inst]] = true;
                }
            }
            // it = gen U (v_old - kill)
            IterativeFramework::removeElements(v_temp2,v_old,v_kill);

            IterativeFramework::unionVect(v_temp,v_temp2);
            (*it->second) = *v_temp;

            v_old = (it->second);

            //cout << "ppp "<< it->first;
            //printVector(it->second);

            delete v_temp;
            delete v_temp2;
            delete v_kill;
        }

        // set out to the last point
        (*bp->out)= *v_old;

    }


    // Go through each line and if value matches a point print out the values
    virtual bool printReachDefResults()
    {
    }

    virtual bool runOnModule(Module& M)
    {


        for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI)
        {
            IterativeFramework *iv= new IterativeFramework(*MI);

            iv->runIterativeFramework( true, &defTransfer);
            //iv->printCFG();
            iv->printProgramPoints();

            delete iv;
        }


        return false;
    }
};

char ReachingDef::ID = 0;
RegisterPass<ReachingDef> X("reach", "15745: Iterative Reaching Definition Analysis");

}
