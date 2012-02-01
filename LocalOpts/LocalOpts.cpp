#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/LLVMContext.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <ostream>
#include <fstream>
#include <iostream>
#include <stack>
#include <map>

using namespace std;
using namespace llvm;

namespace
{

class LocalOpts : public ModulePass
{
   template <class T> void
   clear_stack(stack<T>& stk)
   {
      while(!stk.empty())
         stk.pop();
   }
   
   static inline bool
   is_power_of_two(const int x)
   {
      return (x & (x - 1)) == 0;
   }
   
   static inline int
   find_exponent_power_of_two(int x)
   {
      int log_value = 0;
      
      while (x != 1) {
         log_value++;
         x >>= 1;
      }
      
      return log_value;
   }
   
   ConstantInt*
   generate_zero(Type const * typ)
   {
      return generate_constant(typ, 0);
   }
   
   ConstantInt*
   generate_constant(Type const *typ, int val)
   {
      return ConstantInt::get((IntegerType*)typ, val);
   }
   
   // takes a basic block and performs
   // basic optimizations:
   // x + 0 = x
   // x * 2 = x << 1
   // etc
   void do_simple_optimizations(BasicBlock& blk)
   {
      BasicBlock::iterator it(blk.begin());
      BasicBlock::iterator end(blk.end());
      stack<LoadInst*> ld_stk;
      
      for(; it != end; ++it) {
         Instruction& inst(*it);
         
         if(LoadInst::classof(&inst)) {
            LoadInst *ld((LoadInst*)&inst);
            
            if(!ld_stk.empty()) {
               assert(ld_stk.size() == 1);
               
               LoadInst *top(ld_stk.top());
               
               if(ld->isIdenticalTo(top)) {
                  // here we have two consecutive load instructions
                  // %1 = load x
                  // %2 = load x
                  // we now wait for a div instruction
                  ld_stk.push(ld);
               }
            } else
               ld_stk.push(ld);
            
         } else if(StoreInst::classof(&inst)) {
            StoreInst *si((StoreInst*)&inst);
            if(ld_stk.size() == 1) {
               LoadInst *li(ld_stk.top());
               Value *lp(li->getPointerOperand());
               Value *sp(si->getPointerOperand());
               
               if(sp == lp) {
                  --it; --it;
                  li->eraseFromParent();
                  si->eraseFromParent();
                  cout << "Destroying stupid load/store pair...\n";
               }
            }
            clear_stack(ld_stk);
         } else if(BinaryOperator::classof(&inst)) {
            BinaryOperator *bin((BinaryOperator*)&inst);
            Instruction::BinaryOps op(bin->getOpcode());
            if(ld_stk.size() == 2) {
               switch (op) {
                  case Instruction::SDiv: {
                     Value *v1(bin->getOperand(0));
                     Value *v2(bin->getOperand(1));
                  
                     LoadInst *l2(ld_stk.top());
                     ld_stk.pop();
                     LoadInst *l1(ld_stk.top());
                     ld_stk.pop();
                  
                     if(l1 == v1 && l2 == v2) {
                        l2->eraseFromParent();
                        l1->eraseFromParent();
                        cout << "Simplifying x/x = 1...\n";
                        ReplaceInstWithValue(blk.getInstList(), it, generate_constant(v1->getType(), 1));
                        --it;
                     }
                  }
                  break;
               }
            } else if(ld_stk.size() == 1) {
               switch (op) {
                  case Instruction::SDiv: {
                     Value *v1(bin->getOperand(0));
                     Value *v2(bin->getOperand(1));
                     
                     if(ConstantInt::classof(v2)) {
                        ConstantInt *i((ConstantInt*)v2);
                        if(i->isZero()) {
                           cout << "Simplifying x/0 = 0...\n";
                           
                           LoadInst *l(ld_stk.top());
                           
                           l->eraseFromParent();
                           
                           ReplaceInstWithValue(blk.getInstList(), it,
                              generate_zero(v1->getType()));
                           --it;
                        } else {
                           int val((int)i->getSExtValue());
                           if(is_power_of_two(val)) {
                              cout << "Simplifying x/{2,4,...} = x >> y...\n";
                              BinaryOperator *new_op(BinaryOperator::Create(Instruction::LShr,
                                 v1, generate_constant(v2->getType(), find_exponent_power_of_two(val))));

                              ReplaceInstWithInst(blk.getInstList(), it, new_op);
                           } else if(val == 1) {
                              cout << "Simplifying x/1 = x...\n";
                              ReplaceInstWithValue(blk.getInstList(), it,
                                 v1);
                              --it;
                           }
                        }
                     }
                  }
                  break;
                  case Instruction::Add: {
                     // x + 0 or 0 + x
                     Value *v1(bin->getOperand(0));
                     Value *v2(bin->getOperand(1));
                     
                     if(ConstantInt::classof(v1)) {
                        ConstantInt *i((ConstantInt*)v1);
                        
                        if(i->isZero()) {
                           cout << "Simplifying 0 + x = x...\n";
                           
                           LoadInst *l(ld_stk.top());
                           
                           l->eraseFromParent();
                           ReplaceInstWithValue(blk.getInstList(), it, v2);
                           --it;
                        }
                     } else if(ConstantInt::classof(v2)) {
                        cout << "Simplifying x + 0 = x...\n";
                        ConstantInt *i((ConstantInt*)v2);
                        if(i->isZero()) {
                           LoadInst *l(ld_stk.top());
                           
                           l->eraseFromParent();
                           
                           ReplaceInstWithValue(blk.getInstList(), it, v1);
                           --it;
                        }
                     }
                  }
                  break;
                  case Instruction::Mul: {
                     Value *v1(bin->getOperand(0));
                     Value *v2(bin->getOperand(1));
                     
                     if(ConstantInt::classof(v1))
                        swap(v1, v2);
                     
                     if(ConstantInt::classof(v2)) {
                        ConstantInt *i((ConstantInt*)v2);
                        const int val((int)i->getSExtValue());
                        
                        if(val != 0 && is_power_of_two(val)) {
                           cout << "Simplifying x * {2,4,..} = <<...\n";
                           
                           // x * {2,4,6,...} = x<<X
                           BinaryOperator *new_op(BinaryOperator::Create(Instruction::Shl,
                              v1, generate_constant(v2->getType(), find_exponent_power_of_two(val))));
                           
                           ReplaceInstWithInst(blk.getInstList(), it, new_op);
                           
                        } else if(val == 0) {
                           // x * 0 = 0
                           cout << "Simplifying x * 0 = 0...\n";
                           LoadInst *l(ld_stk.top());
                           
                           l->eraseFromParent();
                           
                           ReplaceInstWithValue(blk.getInstList(), it, generate_zero(v1->getType()));
                           --it;
                        } else if(val == 1) {
                           cout << "Simplifying x * 1 = x...\n";
                           
                           ReplaceInstWithValue(blk.getInstList(), it, v1);
                           --it;
                        } else if(val == 3) {
                           cout << "Simplifying x * 3 = (x << 1) + x...\n";
                           BinaryOperator *new_shl(BinaryOperator::Create(Instruction::Shl,
                              v1, generate_constant(v2->getType(), 1)));
                           BinaryOperator *new_add(BinaryOperator::Create(Instruction::Add,
                              new_shl, v1));
                           
                           blk.getInstList().insert(it, new_shl);
                           
                           ReplaceInstWithInst(blk.getInstList(), it, new_add);
                        }
                     }
                  }
                  break;
               }
               clear_stack(ld_stk);
            } else {
               clear_stack(ld_stk);
            }
         } else {
            clear_stack(ld_stk);
         }
      }
   }
   
   // does constant folding on a basic block
   void do_constant_folding(BasicBlock& blk)
   {  
      BasicBlock::iterator it(blk.begin());
      BasicBlock::iterator end(blk.end());
      
      // stores dictionary between values and its constants
      typedef map<Value*, Value*> vals_map;
      typedef vals_map::iterator vals_iterator;
      vals_map vals;
      
      for(; it != end; ++it) {
         Instruction& inst(*it);
         
         if(StoreInst::classof(&inst)) {
            StoreInst *si((StoreInst*)&inst);
            Value *v(si->getValueOperand());
            Value *p(si->getPointerOperand());
            
            if(ConstantInt::classof(v)) {
               ConstantInt *i((ConstantInt*)v);
               
               // constant changed, let's update it
               vals[p] = v;
               
               // check if next instruction is br label %return
               // if it isn't then we can safely remove this store instruction!
               
               bool next_is_branch(false);
               BasicBlock::iterator saved(it);
               
               saved++;
               
               if(saved != end) {
                  Instruction& next(*saved);
                  
                  if(BranchInst::classof(&next)) {
                     BranchInst *br((BranchInst*)&next);
                     
                     if(br->isUnconditional())
                        next_is_branch = true;
                  }
               }
               
               
               if(!next_is_branch) {
                  cout << "Removing constant store instruction...\n";
                  --it;
                  si->eraseFromParent();
               }
               
            } else {
               vals_iterator result(vals.find(v));
               
               if(result != vals.end()) {
                  // code is trying to store a constant, let's change it
                  StoreInst *new_store(new StoreInst(result->second, p, false, si));
                  
                  ReplaceInstWithInst(blk.getInstList(), it, new_store);
               } else {
                  vals_iterator result(vals.find(p));
                  // we need to remove this reference
                  // because it's no longer constant
                  
                  if(result != vals.end()) {
                     vals.erase(result);
                  }
               }
            }
         } else if(LoadInst::classof(&inst)) {
            
            LoadInst *li((LoadInst*)&inst);
            Value *p(li->getPointerOperand());
            
            vals_iterator result(vals.find(p));
            
            if(result != vals.end()) {
               Value *v(result->second);
               ReplaceInstWithValue(blk.getInstList(), it, v);
               it--;
               
               cout << "Replacing a load with a constant...\n";
            }
            
         } else if(BinaryOperator::classof(&inst)) {
            BinaryOperator *bo((BinaryOperator*)&inst);
            Value *v1(bo->getOperand(0));
            Value *v2(bo->getOperand(1));
            
            if(!ConstantInt::classof(v1) || !ConstantInt::classof(v2))
               continue; // not a constants, just continue to next instruction
               
            switch(bo->getOpcode()) {
               case Instruction::Add: {
                  ConstantInt *i1((ConstantInt*)v1);
                  ConstantInt *i2((ConstantInt*)v2);
                  
                  const int val1((int)i1->getSExtValue());
                  const int val2((int)i2->getSExtValue());
                  
                  ConstantInt *new_val(generate_constant(i1->getType(), val1 + val2));
                  
                  ReplaceInstWithValue(blk.getInstList(), it, new_val);
                  it--;
                  
                  cout << "Adding " << val1 << " + " << val2 << "\n";
               }
               break;
               case Instruction::Mul: {
                  ConstantInt *i1((ConstantInt*)v1);
                  ConstantInt *i2((ConstantInt*)v2);
                  
                  const int val1((int)i1->getSExtValue());
                  const int val2((int)i2->getSExtValue());
                  
                  ConstantInt *new_val(generate_constant(i1->getType(), val1 * val2));
                  
                  ReplaceInstWithValue(blk.getInstList(), it, new_val);
                  --it;
                  
                  cout << "Multiplying " << val1 << " * " << val2 << "\n";
               }
               break;
               case Instruction::Shl: {
                  ConstantInt *i1((ConstantInt*)v1);
                  ConstantInt *i2((ConstantInt*)v2);
                  
                  const int val1((int)i1->getSExtValue());
                  const int val2((int)i2->getSExtValue());
                  
                  ConstantInt *new_val(generate_constant(i1->getType(), val1 << val2));
                  
                  ReplaceInstWithValue(blk.getInstList(), it, new_val);
                  --it;
                  
                  cout << "Left shifting " << val1 << " << " << val2 << "\n";
               }
               break;
               case Instruction::LShr: {
                  ConstantInt *i1((ConstantInt*)v1);
                  ConstantInt *i2((ConstantInt*)v2);
                  
                  const int val1((int)i1->getSExtValue());
                  const int val2((int)i2->getSExtValue());
                  
                  ConstantInt *new_val(generate_constant(i1->getType(), val1 >> val2));
                  
                  ReplaceInstWithValue(blk.getInstList(), it, new_val);
                  --it;
                  
                  cout << "Right shifting " << val1 << " >> " << val2 << "\n";
               }
               break;
               case Instruction::Sub: {
                  ConstantInt *i1((ConstantInt*)v1);
                  ConstantInt *i2((ConstantInt*)v2);
                  
                  const int val1((int)i1->getSExtValue());
                  const int val2((int)i2->getSExtValue());
                  
                  ConstantInt *new_val(generate_constant(i1->getType(), val1 - val2));
                  
                  ReplaceInstWithValue(blk.getInstList(), it, new_val);
                  --it;
                  
                  cout << "Subtracting " << val1 << " - " << val2 << "\n";
               }
               break;
               case Instruction::SDiv: {
                  ConstantInt *i1((ConstantInt*)v1);
                  ConstantInt *i2((ConstantInt*)v2);
                  
                  const int val1((int)i1->getSExtValue());
                  const int val2((int)i2->getSExtValue());
                  
                  ConstantInt *new_val(generate_constant(i1->getType(), val1 / val2));
                  
                  cout << "Dividing " << val1 << " / " << val2 << "\n";
                  
                  ReplaceInstWithValue(blk.getInstList(), it, new_val);
                  --it;
               }
               break;
            }
         }
      }
   }
   
   void remove_stupid_allocs(BasicBlock& blk)
   {
      BasicBlock::iterator it(blk.begin());
      BasicBlock::iterator end(blk.end());
      
      for(; it != end; ++it) {
         Instruction& instr(*it);
         
         if(AllocaInst::classof(&instr)) {
            AllocaInst *alloc((AllocaInst*)&instr);
            
            if(alloc->use_empty()) {
               --it;
               alloc->eraseFromParent();
               cout << "Removing an unused alloc...\n";
            }
         }
      }
   }
  
public:
	static char ID;

	LocalOpts(): ModulePass(ID)
	{
	}

	~LocalOpts()
	{
	}

  // We don't modify the program, so we preserve all analyses
  virtual void getAnalysisUsage(AnalysisUsage &AU) const
  {
     AU.setPreservesCFG();
  }
	
  virtual bool runOnModule(Module& M)
  {
    for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI)
    {
       Function& fun(*MI);

       for(Function::BasicBlockListType::iterator it(fun.begin()), end(fun.end()); it != end; ++it) {
          BasicBlock &blk(*it);
          blk.dump();
          do_simple_optimizations(blk);
          do_constant_folding(blk);
          remove_stupid_allocs(blk);
          blk.dump();
       }  
    }
    return false;
  }
};

char LocalOpts::ID = 0;
RegisterPass<LocalOpts> X("local-opts", "15745: Block Optimizations");
}
