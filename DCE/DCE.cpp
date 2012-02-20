#undef NDEBUG

#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/Instruction.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/LLVMContext.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "Variables.hpp"
#include "IterativeFramework.hpp"
#include "llvm/Support/CFG.h"

#include <queue>
#include <ostream>
#include <fstream>
#include <iostream>
#include <stack>
#include <map>
#include <set>

using namespace std;
using namespace llvm;

namespace
{

class DCE;
DCE *ptr; 
static bitvector doRunPass(CustomBlock&);  

class DCE : public ModulePass
{
public:
	static char ID;

	DCE(): ModulePass(ID)
	{
	}

	~DCE()
	{
	}

  // We don't modify the program, so we preserve all analyses
  virtual void getAnalysisUsage(AnalysisUsage &AU) const
  {
     //AU.setPreservesCFG();
  }
  
  void runModule (BasicBlock &blk)
  {
     BasicBlock::iterator it(blk.begin());
     BasicBlock::iterator end(blk.end());
     
     for(; it != end; ++it) {
        Instruction& inst(*it);
        Value *vl((Value *)&inst);
        
        cout << vl->getName().str() << endl;
     }
  }
  
  bitvector runPass(CustomBlock& cblk)
  {
     // must start reading OUT
     bitvector out(cblk.out);
     
     BasicBlock *blk(cblk.blk);
     
     BasicBlock::InstListType &ls(blk->getInstList());

#define GET_NAME(VALUE) ((VALUE)->getName().str())
#define GET_INDEX(VALUE) (cur->getVarIndex(GET_NAME(VALUE)))
#define IS_VALUE_FAINT(VALUE) out[GET_INDEX(VALUE)]
#define MARK_NOT_FAINT(VALUE) do { \
     cout << "Variable " << GET_NAME(VALUE) << " (" << GET_INDEX(VALUE) << ") no longer faint\n"; \
     out[GET_INDEX(VALUE)] = false;   \
     } while(false)
     
     cout << "==================================================\n";
     
     blk->dump();
     
     cout << "-----------------\n";
        
     // iterate the instructions in reverse order
     for(BasicBlock::InstListType::reverse_iterator it(ls.rbegin()), e(ls.rend()); it != e; it++) {
        Instruction& i(*it);
        
        if(isa<StoreInst>(i)) {
           StoreInst *s((StoreInst*)&i);
           Value *op(s->getValueOperand());
           Value *ptr(s->getPointerOperand()); // destination
           
           // case: ptr = op
           
           if(!isa<Constant>(op)) {
              const size_t idx_ptr(GET_INDEX(ptr));
              
              if(!IS_VALUE_FAINT(ptr)) { // ptr is not faint
                 MARK_NOT_FAINT(op);
              }
           }
        } else if(isa<ReturnInst>(i)) {
           ReturnInst *r((ReturnInst*)&i);
           Value *ret(r->getReturnValue());
           
           if(!isa<Constant>(ret)) {
              // case: return x
              MARK_NOT_FAINT(ret);
           }
        } else if(isa<LoadInst>(i)) {
           LoadInst *l((LoadInst*)&i);
           Value *x(l->getPointerOperand());
           
           // l = load x
           
           if(!IS_VALUE_FAINT(l)) {
              MARK_NOT_FAINT(x);
           }
        } else if(isa<BinaryOperator>(i)) {
           if(!IS_VALUE_FAINT(&i)) {
              for(size_t op(0); op < i.getNumOperands(); ++op) {
                 Value *oper(i.getOperand(op));
                 if(oper && !isa<Constant>(oper)) {
                    MARK_NOT_FAINT(oper);
                 }
              }
           }
        } else if(isa<CmpInst>(i)) {
           if(!IS_VALUE_FAINT(&i)) {
              for(size_t op(0); op < i.getNumOperands(); ++op) {
                 Value *oper(i.getOperand(op));
                 if(oper && !isa<Constant>(oper)) {
                    MARK_NOT_FAINT(oper);
                 }
              }
           }
        } else if(isa<CallInst>(i)) {
           if(!i.getType()->isVoidTy())
              MARK_NOT_FAINT(&i);
           
           CallInst *c((CallInst*)&i);
           for(size_t op(0); op < c->getNumArgOperands(); ++op) {
              Value *arg(c->getArgOperand(op));
              if(arg && !isa<Constant>(arg)) {
                 MARK_NOT_FAINT(arg);
              }
           }
        } else if(isa<AllocaInst>(i)) {
           // x = alloca size
           // ignore
        } else if(isa<BranchInst>(i)) {
           BranchInst *br((BranchInst*)&i);
           
           if(br->isUnconditional()) {
              // skip
           } else {
              Value *cond(br->getCondition());
              if(!isa<Constant>(cond))
                 MARK_NOT_FAINT(cond);
           }
        } else {
           cout << "WARNING!!! THIS INSTRUCTION IS NOT SUPPORTED\n";
           assert(false);
           i.dump();
        }
     }
     
     for(size_t i(0); i < out.size(); ++i) {
       if(out[i]) {
          cout << cur->getVarName(i) << " is faint\n";
       }
     }
     
     // this is the new in
     return out;
  }
  
  typedef set<std::string> set_faint;
  
  static inline bool is_faint(set_faint& set, const string& name)
  {
     set_faint::const_iterator it(set.find(name));
     
     return it != set.end();
  }
  
  void deleteDeadCodeBlock(BasicBlock *blk, set_faint& faint, queue<Instruction*>& to_delete)
  {
     BasicBlock::InstListType &ls(blk->getInstList());
     typedef BasicBlock::InstListType::iterator iter;
     
     for(iter it(ls.begin()), e(ls.end()); it != e;) {
        Instruction& i(*it);
        
        iter next(it);
        ++next;
        
#define VALUE_IS_FAINT(VALUE) is_faint(faint, (VALUE)->getName().str())
#define DELETE_INSTR(TYPE, INSTR) do { \
        /*cout << "DELETE " << TYPE << endl;*/ \
           (INSTR).dump(); \
           (INSTR).removeFromParent(); \
           to_delete.push(&(INSTR)); \
        } while (false)
        
        //i.dump();
        
        if(isa<StoreInst>(i)) {
           StoreInst *s((StoreInst*)&i);
           Value *ptr(s->getPointerOperand());
           
           if(VALUE_IS_FAINT(ptr))
              DELETE_INSTR("store", i);
           
        } else if(isa<LoadInst>(i)) {
           if(VALUE_IS_FAINT(&i)) //cout << "MUST DELETE\n"; i.dump(); }
              DELETE_INSTR("load", i);
              
        } else if(isa<BinaryOperator>(i)) {
           if(VALUE_IS_FAINT(&i))
              DELETE_INSTR("binary", i);
        } else if(isa<AllocaInst>(i)) {
           if(VALUE_IS_FAINT(&i))
              DELETE_INSTR("alloca", i);
        } else if(isa<ReturnInst>(i)) {
           ReturnInst *r((ReturnInst*)&i);
             Value *ret(r->getReturnValue());
             if(!isa<Constant>(ret) && VALUE_IS_FAINT(ret)) {
                DELETE_INSTR("ret", i);
             }
        } else if(isa<CmpInst>(i)) {
           if(VALUE_IS_FAINT(&i))
              DELETE_INSTR("cmp", i);
        } else if(isa<BranchInst>(i)) {
           BranchInst *br((BranchInst*)&i);

           if(br->isUnconditional()) {
                // skip
           } else {
              Value *cond(br->getCondition());
              if(!isa<Constant>(cond) && VALUE_IS_FAINT(cond))
                 DELETE_INSTR("br", i);
           }
        } else if(isa<CallInst>(i)) {
           if(i.getType()->isVoidTy()) {
              // skip
           } else {
               if(VALUE_IS_FAINT(&i))
                  DELETE_INSTR("call", i);
           }
        } else {
           cout << "DONT KNOW HOW TO DELETE THIS INSTRUCTION\n";
           i.dump();
           assert(false);
        }
        
        it = next;
     }
  }
  
  void deleteDeadCode(IterativeFramework& cfg, Variables& vars, Function& fun)
  {
     bitvector in(cfg.getInEntry());
     set_faint faint;

     for(size_t i(0); i < in.size(); ++i) {
        if(in[i]) {
           faint.insert(vars.getVarName(i));
           //cout << vars.getVarName(i) << " is faint\n";
        }
     }

     queue<BasicBlock*> work_list;
     set<BasicBlock*> visited;

     for(Function::iterator i(fun.begin()), e = fun.end(); i != e; ++i) {
        if(isa<ReturnInst>(i->getTerminator())) {
           BasicBlock *blk(&*i);
           work_list.push(blk);
           visited.insert(blk);
        }
     }

     cout << fun.size() << endl;

     queue<Instruction*> to_delete;

     while (!work_list.empty()) {
        BasicBlock *blk(work_list.front());
        work_list.pop();

        deleteDeadCodeBlock(blk, faint, to_delete);

        for(pred_iterator pi(pred_begin(blk)), e(pred_end(blk)); pi != e; pi++) {
           BasicBlock *next(*pi);
           set<BasicBlock*>::const_iterator f(visited.find(next));

           if(f == visited.end()) {
              work_list.push(next);
              visited.insert(next);
           }
        }
     }

       // now delete everything there is to delete
     while(!to_delete.empty()) {
        Instruction *i(to_delete.front());
        to_delete.pop();

        delete i;
     }
  }

  virtual bool runOnModule(Module& M)
  {
     ptr = this;
     
    for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI)
    {
       Function& fun(*MI);
       
       if(fun.empty())
          continue;

       Variables vars(fun);
       
       IterativeFramework cfg(fun, IterativeFramework::DIRECTION_BACKWARD, IterativeFramework::MEET_INTERSECTION,
          doRunPass, vars.getUniversalSet());
       
       cur = &vars;
       cfg.execute();
       
       cout << "Removing dead code...\n";
       deleteDeadCode(cfg, vars, fun);
    }
    return true;
  }
  
private:
   
   Variables *cur;
};

char DCE::ID = 0;
RegisterPass<DCE> X("dce-pass", "15745: Dead Code Elimination");

static bitvector
doRunPass(CustomBlock& cblk)
{
   return ptr->runPass(cblk);
}
}
