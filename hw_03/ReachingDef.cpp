#include "IterativeFramework.hpp"
#include "Variables.hpp"

#include "llvm/BasicBlock.h"
#include "llvm/Constants.h"
#include "llvm/LLVMContext.h"
#include "llvm/Analysis/LoopInfo.h"
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


#define GET_NAME1(VALUE) ((VALUE)->getName().str())
#define GET_INDEX1(VALUE) (inv_var->getVarIndex(GET_NAME1(VALUE)))
#define INV_VALUE(VALUE) do {                                           \
            cout << "Variable " << GET_NAME1(VALUE) << " (" << GET_INDEX1(VALUE) << ") is invariant\n"; \
            out[GET_INDEX1(VALUE)] = true;                               \
        } while(false)


#define GET_NAME2(VALUE) ((VALUE)->getName().str())
#define GET_INDEX2(VALUE) (cur->getVarIndex(GET_NAME2(VALUE)))
#define IS_DEFINED(VALUE,POINT) (POINT[GET_INDEX2(VALUE)])

#define GEN_VALUE(VALUE) do {                                           \
            cout << "Variable " << GET_NAME2(VALUE) << " (" << GET_INDEX2(VALUE) << ") was generated\n"; \
            out[GET_INDEX2(VALUE)] = true;                               \
        } while(false)
#define KILL_VALUE(VALUE) do {                                          \
            cout << "Variable " << GET_NAME2(VALUE) << " (" << GET_INDEX2(VALUE) << ") was killed\n"; \
            out[GET_INDEX2(VALUE)] = false;                              \
        } while(false)


using namespace llvm;
using namespace std;

namespace
{

class ReachingDef;
ReachingDef *ptr;
static bitvector doRunPass(CustomBlock&);
static bitvector doRunPassForIV(CustomBlock&);

class ReachingDef : public ModulePass
{

private:

    // For reaching def
    Variables *cur;

    // for invariant detection
    Variables *inv_var;
    IterativeFramework *globalRD;


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
        AU.addRequired<LoopInfo>();
//        AU.addRequired<DominatorTree>();
    }

    // At the block
    // Check if this block is in a loop.
    // if it is not pass
    // else lets determine whether it is an invariant
    bitvector doIsInvariantPass(CustomBlock& cblk)
    {
        LoopInfo &LI = getAnalysis<LoopInfo>(*(cblk.blk->getParent()));
        Loop *loop   = LI.getLoopFor(cblk.blk);


        // must start reading IN
        bitvector out(cblk.in);

        // We need the reaching def info for this block
        bitvector rdIn(globalRD->getMap(cblk.blk)->in);
        //bitvector rdIn(globalRD->getMap(cblk.blk->getSinglePredecessor())->in);





        //LoopInfo LI;

        BasicBlock *blk(cblk.blk);

        if(loop  == NULL)
        {

            // nothing has changed
            return cblk.out;
        }


        BasicBlock::InstListType &ls(blk->getInstList());


        cout << "==================================================\n";

        blk->dump();

        cout << "-----------------\n";

        // iterate the instructions in reverse order
        for(BasicBlock::InstListType::iterator it(ls.begin()), e(ls.end()); it != e; it++)
        {
            User::op_iterator OI, OE;
            bool instIsInv = true;

            // if this a void type skip it:
            if(it->getType()->isVoidTy())
                continue;

#if 1
            // for testing purposes we will use the built-in loop inv testing functionality
            if(!loop->hasLoopInvariantOperands(it))
                instIsInv = false;
#else
            for(OI = it->op_begin(), OE = it->op_end(); OI != OE; ++OI)
            {
                Value *val = *OI;

                // These are the arguments
                if(isa<Instruction>(val) || isa<Argument>(val))
                {
                    // we need to test if IT is invariant:
                    // all of its arguments are defined outside the block, or by other invariants.


                    // is this argument already defined at entry time?
                    // is this variable invariant?
                    if(IS_DEFINED(val,rdIn))
                    {
                        instIsInv&=true;

                    }
                    else
                    {
                        instIsInv&=false;
                        break;
                    }

                }


            }
#endif
            if(instIsInv)
            {
                assert(!it->getType()->isVoidTy());
                cout << "void type: " << it->getType()->isVoidTy() << endl;
                cout << "zomg this is working: "<< GET_NAME1(it) << endl;
                // Add this to the INV_VALUE
                INV_VALUE(it);
            }
            if(loop->hasLoopInvariantOperands(it))
                cout << "This is really loop invariant: : "<< GET_NAME1(it) << endl;

        }


        // this is the new out
        return out;



    }

    // At the block
    bitvector doReachDefPass(CustomBlock& cblk)
    {
        // must start reading IN, We just named it out because
        // this is what we will return.
        bitvector out(cblk.in);

        BasicBlock *blk(cblk.blk);

        BasicBlock::InstListType &ls(blk->getInstList());

        cout << "==================================================\n";

        blk->dump();

        cout << "-----------------\n";

        for(BasicBlock::InstListType::iterator it(ls.begin()), e(ls.end()); it != e; it++) {
            Instruction& i(*it);

            if(isa<StoreInst>(i)) {
                StoreInst *s((StoreInst*)&i);
                Value *op(s->getValueOperand());
                Value *ptr(s->getPointerOperand()); // destination

                // case: ptr = op
                // Store does nothing

            } else if(isa<ReturnInst>(i)) {
                ReturnInst *r((ReturnInst*)&i);
                Value *ret(r->getReturnValue());

                // Return values not included
                //GEN_VALUE(r);

            } else if(isa<LoadInst>(i)) {
                LoadInst *l((LoadInst*)&i);
                Value *x(l->getPointerOperand());

                // l = load x
                // GEN the instruction
                GEN_VALUE(l);

            } else if(isa<BinaryOperator>(i)) {
                BinaryOperator *b((BinaryOperator*)&i);
                // Gen the result of this operation
                GEN_VALUE(b);

                for(size_t op(0); op < i.getNumOperands(); ++op) {
                    Value *oper(i.getOperand(op));

                    if(oper && !isa<Constant>(oper)) {
                        // MARK_NOT_FAINT(oper);
                        // nothing is gen or killed here
                    }

                }
            } else if(isa<CmpInst>(i)) {
                CmpInst *c((CmpInst*)&i);

                GEN_VALUE(c);

                // We are generating the result of this op
                for(size_t op(0); op < i.getNumOperands(); ++op) {
                    Value *oper(i.getOperand(op));
                    if(oper && !isa<Constant>(oper)) {
                        //MARK_NOT_FAINT(oper);
                    }

                }
            } else if(isa<CallInst>(i)) {
                CallInst *c((CallInst*)&i);

                GEN_VALUE(c);

                // If there is a return value it is generated.


            } else if(isa<AllocaInst>(i)) {
                // x = alloca size
                // ignore
            } else if(isa<BranchInst>(i)) {
                BranchInst *br((BranchInst*)&i);
                // Do nothing

            } else if(isa<PHINode>(i)) {
                PHINode *node((PHINode*)&i);

                for(size_t val(0); val < node->getNumIncomingValues(); val++) {
                    Value *value(node->getIncomingValue(val));
                    if(!isa<Constant>(value)) {
                        // We are KILLING these values
                        KILL_VALUE(value);
                    }
                }
            } else {
                cout << "WARNING!!! THIS INSTRUCTION IS NOT SUPPORTED\n";
                i.dump();
                assert(false);
            }
        }

        // this is the new out
        return out;


    }


    // Go through each line and if value matches a point print out the values
    virtual bool printReachDefResults()
    {
    }

    virtual bool runOnModule(Module& M)
    {

        ptr = this;


//            DominatorTree &DT =  getAnalysis<DominatorTree>(fun);
//            LoopInfo &LI      =  getAnalysis<LoopInfo>();

        for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI)
        {
            Function& fun(*MI);

            if(fun.empty())
                continue;

            // for reaching def
            Variables vars(fun);

            // for invariant
            Variables vars2(fun);

            IterativeFramework rd(fun,IterativeFramework::DIRECTION_FORWARD,
                                  IterativeFramework::MEET_UNION, doRunPass,
                                  vars.getFlagedFunctionArgs() );

            IterativeFramework iv(fun,IterativeFramework::DIRECTION_FORWARD,
                                  IterativeFramework::MEET_UNION, doRunPassForIV,
                                  vars2.getEmptySet() );


            cur = &vars;
            rd.execute();

            // We need to keep rd so we can pass it to IV
            globalRD = &rd;


            cout <<endl<< "=================== IV PHASE =======================" <<endl;


            iv.execute();

            cout <<endl<< "================= END IV PHASE =====================" <<endl;
        }


        return false;
    }

};

static bitvector doRunPass(CustomBlock& cblk)
{
    return ptr->doReachDefPass(cblk);
}

static bitvector doRunPassForIV(CustomBlock& cblk)
{
    return ptr->doIsInvariantPass(cblk);
}


char ReachingDef::ID = 0;
RegisterPass<ReachingDef> X("reach", "15745: Iterative Reaching Definition Analysis");

}

#undef GET_NAME1
#undef GET_INDEX1
#undef INV_VALUE

#undef GET_NAME2
#undef GET_INDEX2
#undef GEN_VALUE
#undef KILL_VALUE
