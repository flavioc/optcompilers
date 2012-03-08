#include "IterativeFramework.hpp"

#include "llvm/ADT/Twine.h"
#include "llvm/BasicBlock.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/InstrTypes.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/User.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CFG.h"


#include <queue>
#include <iostream>
#include <list>
#include <map>
#include <ostream>
#include <string>
#include <vector>

using namespace llvm;
using namespace std;


IterativeFramework::IterativeFramework(Function &F, direction_t dir,
   meet_operator_t mt, transfer_function tran, bitvector init):
   fun(&F), direction(dir), meet(mt), transfer(tran)
{
   buildDataFlowGraph(F, init);
}

IterativeFramework::~IterativeFramework()
{
   for(list<CustomBlock*>::iterator it(cfg.nodes.begin()), end(cfg.nodes.end()); it != end; it++)
      delete *it;
}

void
IterativeFramework::buildDataFlowGraph(Function& F, bitvector& init)
{
   typedef Function::BasicBlockListType::iterator block_iter;
   
   block_iter it(F.begin());
   block_iter end(F.end());
   
   for(; it != end; ++it) {
      BasicBlock& blk(*it);
      CustomBlock *new_blk(new CustomBlock);
      
      new_blk->blk = &blk;
      new_blk->in = init;
      new_blk->out = init;
      new_blk->first_time = true;
      new_blk->in_queue = false;
      
      cfg.nodes.push_back(new_blk);
      cfg.mapping[&blk] = new_blk;
   }
}

CustomBlock *
IterativeFramework::getMap(BasicBlock *blk)
{
   mapping_map::iterator f(cfg.mapping.find(blk));
   assert(f != cfg.mapping.end());
   
   return f->second;
}

const CustomBlock *
IterativeFramework::getMap(BasicBlock *blk) const
{
   mapping_map::const_iterator f(cfg.mapping.find(blk));
   assert(f != cfg.mapping.end());
   
   return f->second;
}

bitvector
IterativeFramework::doMeetWithOperator(meet_operator_t meet, bitvector& a, bitvector& b)
{
   switch(meet) {
      case MEET_UNION:
          return unionVect(a, b);
      case MEET_INTERSECTION:
          return intersectVect(a, b);
      default:
         assert(false);
   }
   
   assert(false);
}

#define ADD_TO_WORKLIST(LIST, EL) \
   do {  \
      CustomBlock *tmp(EL); \
      if(!tmp->in_queue) { \
         (LIST).push(tmp); \
         tmp->in_queue = true; \
      }  \
   } while(false)

void
IterativeFramework::execute(void)
{
   queue<CustomBlock*> work_list;
   
   // add everything for now.
   // optimal would be: for forward direction
   // we would add the entry node
   // and for the backward direction, the exit nodes
   if(direction == DIRECTION_FORWARD)
      ADD_TO_WORKLIST(work_list, getMap(&fun->getEntryBlock()));
   else if(direction == DIRECTION_BACKWARD) {
      for(Function::iterator i(fun->begin()), e = fun->end(); i != e; ++i) {
         if(isa<ReturnInst>(i->getTerminator())) {
            ADD_TO_WORKLIST(work_list, getMap(i));
         }
      }
   }
   
   while(!work_list.empty()) {
      
      CustomBlock *cblk(work_list.front());
      work_list.pop();
      cblk->in_queue = false;
      
      BasicBlock *blk(cblk->blk);
      
      if(direction == DIRECTION_FORWARD) {
         // meet all predecessors
         pred_iterator pi(pred_begin(blk));
         
         // may not have predecessors, ie, entry point
         if(pi != pred_end(blk)) {
            bitvector new_in(getMap(*pi)->out);
         
            pi++;
         
            for(pred_iterator e = pred_end(blk); pi != e; pi++)
               new_in = doMeetWithOperator(meet, new_in, getMap(*pi)->out);
            
            cblk->in = new_in;
         }
         
         bitvector new_out = transfer(*cblk);
         
         if(new_out != cblk->out || cblk->first_time) {
            // add all successors
            cblk->out = new_out;
            for (succ_iterator si = succ_begin(blk), e = succ_end(blk); si != e; si++)
                 ADD_TO_WORKLIST(work_list, getMap(*si));
            cblk->first_time = false;
         }
      } else if(direction == DIRECTION_BACKWARD) {
         // meet all successors
         succ_iterator si(succ_begin(blk));
         
         // may not have successors, ie, exit point
         if(si != succ_end(blk)) {
            bitvector new_out(getMap(*si)->in);
            int total(1);
            si++;
         
            // fold INs into new OUT
            for(succ_iterator e = succ_end(blk); si != e; si++, total++)
               new_out = doMeetWithOperator(meet, new_out, getMap(*si)->in);
            
            cblk->out = new_out;
         }
         
         bitvector new_in = transfer(*cblk);
         
         if(new_in != cblk->in || cblk->first_time) {
            // add all successors
            cblk->in = new_in;
            int total(0);
            for(pred_iterator pi = pred_begin(blk), e = pred_end(blk); pi != e; pi++, total++)
               ADD_TO_WORKLIST(work_list, getMap(*pi));
            cblk->first_time = false;
         }
      } else
         assert(false);
   }
   
   // do sanity checks
   for(list<CustomBlock*>::iterator it(cfg.nodes.begin()), e(cfg.nodes.end()); it != e; ++it) {
      CustomBlock *cblk(*it);
      assert(!cblk->first_time); // must have run through all the blocks
      assert(!cblk->in_queue); // blocks must not be in the queue since it's now empty
   }
}

bitvector
IterativeFramework::getInEntry(void) const
{
   if(fun->empty())
      return bitvector(); // return empty...
      
   BasicBlock& blk(fun->getEntryBlock());
   
   const CustomBlock *cblk(getMap(&blk));
   return cblk->in;
}

// What follows are static funtions for bit manipulation on vectors.

void
IterativeFramework::removeElements(bitvector &vr, bitvector& v1, bitvector& v2)
{
   for(int i=0; i < v1.size();i++)
      if(v2[i])
      vr[i]=false;
   else
      vr[i] = v1[i];
}

void
IterativeFramework::setEmpty(bitvector& v)
{
   for(int i=0; i < v.size();i++)
    v[i]=false;
}

bitvector
unionVect(bitvector& a, bitvector& b)
{
   size_t num(a.size());

   assert(a.size() == b.size());

   bitvector ret(num, false);

   for(size_t i(0); i < num; i++)
      ret[i] = a[i] || b[i];

   return ret;
}

bitvector
IterativeFramework::intersectVect(bitvector& a, bitvector& b)
{
   size_t num(a.size());
   
   assert(a.size() == b.size());
   
   bitvector ret(num, false);
   
   for(size_t i(0); i < num; i++)
      ret[i] = a[i] && b[i];
   
   return ret;
}

void IterativeFramework::printVector(bitvector& v)
{
   cout << "[";
   for(int i=0; i < v.size();i++)
      cout << " " <<v[i];
   cout << "]" << endl;
}
