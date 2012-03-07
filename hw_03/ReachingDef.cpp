#undef NDEBUG

#include "IterativeFramework.hpp"
#include "Variables.hpp"
#include "BlockVariables.hpp"

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
#define IS_INV(VALUE) (out[GET_INDEX1(VALUE)])
#define INV_VALUE(VALUE) do {                                           \
        cout << "Variable " << GET_NAME1(VALUE) << " (" << GET_INDEX1(VALUE) << ") is invariant\n"; \
        out[GET_INDEX1(VALUE)] = true;                                  \
    } while(false)

#define GET_NAME_DOM(VALUE) ((VALUE)->getName().str())
#define GET_INDEX_DOM(VALUE) (globalBlkVars->getVarIndex(GET_NAME_DOM(VALUE)))
#define ADD_DOM(VALUE) do {                                           \
        cout << "Block " << GET_NAME_DOM(VALUE) << " (" << GET_INDEX_DOM(VALUE) << "/"<< out.size() << ") dominates\n"; \
        out[GET_INDEX_DOM(VALUE)] = true;                                \
    } while(false)


#define GET_NAME2(VALUE) ((VALUE)->getName().str())
#define GET_INDEX2(VALUE) (cur->getVarIndex(GET_NAME2(VALUE)))
#define IS_DEFINED(VALUE,POINT) (POINT[GET_INDEX2(VALUE)])

#define GEN_VALUE(VALUE) do {                                           \
        cout << "Variable " << GET_NAME2(VALUE) << " (" << GET_INDEX2(VALUE) << ") was generated\n"; \
        out[GET_INDEX2(VALUE)] = true;                                  \
    } while(false)
#define KILL_VALUE(VALUE) do {                                          \
        cout << "Variable " << GET_NAME2(VALUE) << " (" << GET_INDEX2(VALUE) << ") was killed\n"; \
        out[GET_INDEX2(VALUE)] = false;                                 \
    } while(false)


using namespace llvm;
using namespace std;

namespace
{

class ReachingDef;
ReachingDef *ptr;
static bitvector doRunPass(CustomBlock&);
static bitvector doRunPassForIV(CustomBlock&);
static bitvector doPromotePassForIV(CustomBlock&);
static bitvector doDomPass(CustomBlock&);

class ReachingDef : public ModulePass
{

private:

    // For reaching def
    Variables *cur;

    BlockVariables *globalBlkVars;
    // for invariant detection
    Variables *inv_var;
    IterativeFramework *globalDom;
    IterativeFramework *globalRD;
    IterativeFramework *globalIV;
    map<BasicBlock*,BasicBlock*> *globalPredDominatorTree;

    Function *globalFun;

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

        AU.addRequired<DominatorTree>();

        AU.addRequired<LoopInfo>();
    }


    // x dom y -> domGraph[x]=y
    multimap<BasicBlock*,BasicBlock*> domGraph;

// Adapted from: http://163.25.101.87/~yen3/wiki/doku.php?id=llvm:llvm_notes
    virtual multimap<BasicBlock*,BasicBlock*> printDomGraph(Function &F)
    {
        DominatorTree &DT = getAnalysis<DominatorTree>(F);

        cout << "================= Creating DOM BI =================" << endl;

        for(Function::iterator bbi = F.begin(), bbie = F.end(); bbi != bbie; bbi++){
            BasicBlock *BBI = bbi;
            for(Function::iterator bbj = F.begin(), bbje = F.end(); bbj != bbje; bbj++){
                BasicBlock *BBJ = bbj;
                if(BBI != BBJ && DT.dominates(BBI, BBJ))
                {
                    errs() << BBI->getName() << " doms " << BBJ->getName()<< "\n";
                    domGraph.insert(pair<BasicBlock*, BasicBlock*>(BBI,BBJ));
                }
            }
        }
        cout << "================ End Creating DOM BI ===============" << endl;

        return domGraph;
    }

// takes what was made in the iterative pass and converts it to something more useful
    // uses globalDom
    virtual multimap<BasicBlock*,BasicBlock*> convertDomVectsIntoGraph(Function &F)
    {
        cout << "================= Creating DOM BI =================" << endl;

        for(Function::iterator bbi = F.begin(), bbie = F.end(); bbi != bbie; bbi++){
            BasicBlock *BBI = bbi;
            for(Function::iterator bbj = F.begin(), bbje = F.end(); bbj != bbje; bbj++){
                BasicBlock *BBJ = bbj;
                CustomBlock *first = globalDom->getMap(BBI);
                if(BBI != BBJ)
                {
                    CustomBlock *second = globalDom->getMap(BBJ);
                    bitvector dominators(second->out);

                    if(dominators[GET_INDEX_DOM(BBI)])
                    {
                        errs() <<""<< BBI->getName()  << " doms " << BBJ->getName()<< "\n";
                        domGraph.insert(pair<BasicBlock*, BasicBlock*>(BBI,BBJ));

                    }
                }
            }
        }
        cout << "================ End Creating DOM BI ===============" << endl;

        return domGraph;
    }

    virtual map<BasicBlock*,BasicBlock*> doDFSOnDomGraph(multimap<BasicBlock*,BasicBlock*> domGraph,Function &F)
    {
        // x dom y -> dominator[x]
        multimap<BasicBlock*,BasicBlock*> dominatorTree(domGraph);
        map<BasicBlock*,BasicBlock*> predDominatorTree;


        // now for some DFS magic
        list<BasicBlock*> st;
        map<BasicBlock*, bool> visited;

        BasicBlock *entry = &F.getEntryBlock();

        cout << "================= Computing DFS =================" << endl;



        visited[entry]=true;
        st.push_back(entry);

        while(!st.empty())
        {
            multimap<BasicBlock*,BasicBlock*>::iterator it;
            BasicBlock *t = st.back();
            st.pop_back();

            for ( it=dominatorTree.begin() ; it != dominatorTree.end(); it++ )
                if(!visited[it->second])
                {
                    BasicBlock *v = (BasicBlock*)(it->second);
                    predDominatorTree[v]=t;
                    st.push_back(v);
                    visited[v] = true;

                    errs() << t->getName() <<  " -> " << v->getName() << "\n";

                }

        }
        cout << "=========== Computing DFS Complete ==============" << endl;

        return predDominatorTree;
    }


    bitvector doComputeDom(CustomBlock& cblk)
    {
        BasicBlock *blk(cblk.blk);

        cout << "==================================================\n";

        // if this is the boundary we want to du something special
        if(blk == globalFun->begin())
        {
            bitvector out(globalBlkVars->getEmptySet());
            ADD_DOM(blk);
            return out;
        }
        else
        {

            // This vector is for flagging invariants.
            bitvector out(cblk.in);

            ADD_DOM(blk);

            for(int i =0; i < out.size(); i++)
                errs() << out[i];
            errs() << "\n";

            // this is the new out
            return out;
        }
    }


    bitvector doPromoteInvariantPass(CustomBlock& cblk)
    {
        BasicBlock *blk(cblk.blk);
        BasicBlock *immediateDomBlock;

        // This vector is for flagging invariants.
        bitvector out(cblk.in);

        // now we need the dominator
        immediateDomBlock = (*globalPredDominatorTree)[blk];

        // We are not in a loop
        if(immediateDomBlock == NULL )
            return out;

        Instruction *domLastInst = &immediateDomBlock->back();

        // We want the out of the reaching definitions for the dominator block
        bitvector definedByDominators(globalRD->getMap(immediateDomBlock)->out);
        BasicBlock::InstListType &ls(blk->getInstList());

        // We need the invariant information from the previous pass
        CustomBlock *ivcblk = globalIV->getMap(blk);
        bitvector ivVect(ivcblk->out);

        list<Instruction*> toBePromoted;


        cout << "==================================================\n";
        cout << "-------- OLD ---------\n";
        immediateDomBlock->dump();
        blk->dump();
        cout << "------ END OLD -------\n\n";

        // iterate the instructions in order
        for(BasicBlock::InstListType::iterator it(ls.begin()), e(ls.end()); it != e; it++)
        {
            User::op_iterator OI, OE;

#if 0 // i need to revisit this
            // need to handle stores and loads with care
            if(isa<StoreInst>(it))
            {
                StoreInst *li = (StoreInst*)&it;

            }
            else if(isa<LoadInst>(it))
            {

            }
#endif
            // if this a void type skip it:
            if(it->getType()->isVoidTy())
                continue;

            // we want to check if this instruction was flagged in the
            // IV pass.
            if(IS_DEFINED(it,ivVect))
                toBePromoted.push_back(it);
        }


        // We are going to take the list of instructions and promote them
        for(list<Instruction*>::iterator it(toBePromoted.begin()), e(toBePromoted.end());it != e; ++it)
        {
            Instruction *inst = (Instruction*)*it;

            inst->moveBefore(domLastInst);

        }

        cout << "-------- NEW ---------\n";
        immediateDomBlock->dump();
        blk->dump();
        cout << "------ END NEW -------\n";



        // this is the new out
        return out;
    }

    // At the block
    // Check if this block is in a loop.
    // if it is not pass
    // else lets determine whether it is an invariant
    bitvector doIsInvariantPass(CustomBlock& cblk)
    {
        LoopInfo &LI = getAnalysis<LoopInfo>(*(cblk.blk->getParent()));
        BasicBlock *blk(cblk.blk);
        Loop *loop   = LI.getLoopFor(blk);
        BasicBlock *immediateDomBlock;

        // This vector is for flagging invariants.
        bitvector out(cblk.in);
        if(loop  == NULL)
            // nothing has changed
            return cblk.out;

        // now we need the dominator
        immediateDomBlock = (*globalPredDominatorTree)[blk];
        assert(immediateDomBlock != NULL);

        // We want the out of the reaching definitions for the dominator block
        bitvector definedByDominators(globalRD->getMap(immediateDomBlock)->out);
        BasicBlock::InstListType &ls(blk->getInstList());

        cout << "==================================================\n";

        blk->dump();

        cout << "-----------------\n";

        // iterate the instructions in order
        for(BasicBlock::InstListType::iterator it(ls.begin()), e(ls.end()); it != e; it++)
        {
            User::op_iterator OI, OE;
            bool instIsInv = true;

#if 0 // i need to revisit this

            // need to handle stores and loads with care
            if(isa<StoreInst>(it))
            {
                StoreInst *li = (StoreInst*)&it;
                //cout << "store: " << IS_DEFINED(li->getPointerOperand(),definedByDominators) << endl;
                errs() << "store: " << li->getPointerOperand()->getName() << "\n";
            }
            else if(isa<LoadInst>(it))
            {
                //LoadInst *li = (LoadInst*)&it;
                //cout << "load: " << IS_DEFINED(li->getPointerOperand(),definedByDominators) << endl;
            }

#endif

            // need to handle phinodes with care
            if(isa<PHINode>(it))
                continue;

            // if this a void type skip it:
            if(it->getType()->isVoidTy())
                continue;

            for(OI = it->op_begin(), OE = it->op_end(); OI != OE; ++OI)
            {
                Value *val = *OI;

                // These are the arguments
                if(isa<Instruction>(val) || isa<Argument>(val))
                    // we need to test if IT is invariant:
                    // all of its arguments are defined outside the block, or by other invariants.
                    // is this argument already defined at entry time?
                    // is this variable invariant?
                    if(IS_DEFINED(val,definedByDominators)|| IS_INV(val))
                        instIsInv&=true;
                    else
                    {
                        instIsInv&=false;
                        break;
                    }
            }
            if(instIsInv)
                INV_VALUE(it);
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
            }
            else {
                //cout << "WARNING!!! THIS INSTRUCTION IS NOT SUPPORTED\n";
                //i.dump();
                //assert(false);
            }
        }

        for(int i =0; i < out.size(); i++)
            errs() << out[i];
        errs() << "\n";

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

        for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI)
        {
            Function& fun(*MI);

            globalFun = &fun;

            if(fun.empty())
                continue;

            cout <<endl<< "=================== REACH DEF PHASE =======================" <<endl;
            // for reaching def
            Variables vars(fun);
            cur = &vars;

            IterativeFramework rd(fun,IterativeFramework::DIRECTION_FORWARD,
                                  IterativeFramework::MEET_UNION, doRunPass,
                                  vars.getFlagedFunctionArgs() );
            rd.execute();
            // We need to keep rd so we can pass it to IV
            globalRD = &rd;
            cout <<endl<< "================= END REACH DEF PHASE =====================" <<endl;


            cout <<endl<< "================== COMPUTE DOM GRAPH =======================" <<endl;
            BlockVariables blkVars(fun);
            globalBlkVars = &blkVars;
            IterativeFramework domPass(fun,IterativeFramework::DIRECTION_FORWARD,
                                       IterativeFramework::MEET_INTERSECTION, doDomPass,
                                       blkVars.getUniversalSet() );
            domPass.execute();
            globalDom = &domPass;

            // TODO: I need to compute the dominator graph
//            multimap<BasicBlock*,BasicBlock*> domGraph(printDomGraph(fun));
            multimap<BasicBlock*,BasicBlock*> domGraph(convertDomVectsIntoGraph(fun));

            // DFS Stage
            map<BasicBlock*,BasicBlock*> predDominatorTree(doDFSOnDomGraph(domGraph,fun));

            globalPredDominatorTree = &predDominatorTree;


            cout <<endl<< "================ END COMPUTE DOM GRAPH =====================" <<endl;






            cout <<endl<< "=================== IV DETECT PHASE =======================" <<endl;
            // for invariant
            Variables vars2(fun);
            // stash these pointers in some globals.
            inv_var = &vars2;


            IterativeFramework iv(fun,IterativeFramework::DIRECTION_FORWARD,
                                  IterativeFramework::MEET_UNION, doRunPassForIV,
                                  vars2.getEmptySet() );
            iv.execute();
            globalIV = &iv;

            cout <<endl<< "================= END IV DETECT PHASE =====================" <<endl;

            cout <<endl<< "=================== IV PROMOTE PHASE =======================" <<endl;

            IterativeFramework ivprm(fun,IterativeFramework::DIRECTION_FORWARD,
                                  IterativeFramework::MEET_UNION, doPromotePassForIV,
                                  vars2.getEmptySet() );
            ivprm.execute();

            cout <<endl<< "================= END IV PROMOTE PHASE =====================" <<endl;

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

static bitvector doPromotePassForIV(CustomBlock& cblk)
{
    return ptr->doPromoteInvariantPass(cblk);
}

static bitvector doDomPass(CustomBlock& cblk)
{
    return ptr->doComputeDom(cblk);
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
