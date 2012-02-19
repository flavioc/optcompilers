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


#include <iostream>
#include <list>
#include <map>
#include <ostream>
#include <string>
#include <vector>

using namespace llvm;
using namespace std;


IterativeFramework::IterativeFramework(Function &F, graph_type_t gra, direction_t dir):
   direction(dir), graph(gra)
    {
        BBtoBlockPoint = new map<BasicBlock*,blockPoints*>();
        valuesToIndex  = new map<Value*,unsigned>();
        CFG = new cfg;

        beginBlock = F.begin();
        endBlock   = F.end();

        buildCFG(F);
        buildBlockPointMap(CFG,BBtoBlockPoint ,valuesToIndex);

	}

IterativeFramework::~IterativeFramework()
	{
        delete CFG;
        delete valuesToIndex;
        delete BBtoBlockPoint;

        // ERROR: I'm not actually deleting the values inside these structures
        //        this should be done.

	}


    // TODO: I need more arguments here
    // For now let's just implement reaching defs to make sure it works.
 bool IterativeFramework::runIterativeFramework(transfer_function transfer
                                        /*, meet_op, set_boundary, set_initial*/)
    {
        // need a modified blocks list for the while loop.
        bool somethingModified=true;

        // Initialize Boundary conditions.
        // +we want to set out[entry]
        if(direction == DIRECTION_FORWARD)
        {
            setEmpty((*BBtoBlockPoint)[CFG->start]->out); //reach-def
            // but actually we do want the args
            for(Function::ArgumentListType::iterator ag=CFG->F->arg_begin(); ag != CFG->F->arg_end(); ++ag)
                (*BBtoBlockPoint)[CFG->start]->out[(*valuesToIndex)[ag]] = true;
        }
        else
        {
            setEmpty((*BBtoBlockPoint)[CFG->start]->in);
        }


        // NOTE: This only sets the internals nodes to EMPTY. Instead it should be set to something defined,
        //       by this functions parameters.
        //
        // Init the outs/ins for the internal nodes depending on direction.
        if(direction == DIRECTION_FORWARD)
            for(list<BasicBlock*>::iterator block=CFG->vertices.begin(); block != CFG->vertices.end(); ++block)
                setEmpty((*BBtoBlockPoint)[*block]->out);
        else
            for(list<BasicBlock*>::iterator block=CFG->vertices.begin(); block != CFG->vertices.end(); ++block)
                setEmpty((*BBtoBlockPoint)[*block]->in);

        // Iterate
        while(somethingModified)
        {
            somethingModified=false;

            for(list<BasicBlock*>::iterator block=CFG->vertices.begin(); block != CFG->vertices.end(); ++block)
            {
                // for each BB other than entry, so including exit?
                if(direction == DIRECTION_FORWARD)
                {
                    //tmp = out[b] we want to detect changes
                    bitvector tmp = (*BBtoBlockPoint)[*block]->out;

                    // We want to union all of the outs of the preds
                    // in[B] = U (out[p])
                    for ( multimap<BasicBlock*, BasicBlock* >::iterator it=CFG->bb_edges.begin() ; it != CFG->bb_edges.end(); it++ )
                        if(it->second == *block)
                        {
                            // in[b] = in[b] U (out[p])
                            unionVect((*BBtoBlockPoint)[*block]->in, (*BBtoBlockPoint)[it->first]->out);
                        }



                    // out = F_b(in[B])

                    transfer((*BBtoBlockPoint)[*block], valuesToIndex);

                    // if tmp differs from  out then we flag a modify
                    if(tmp!=((*BBtoBlockPoint)[*block]->out))
                        somethingModified = true;

                }
                else // not from top
                {
                    //tmp = out[b] we want to detect changes
                    bitvector tmp = (*BBtoBlockPoint)[*block]->in;

                    // We want to union all of the outs of the preds
                    // in[B] = U (out[p])
                    for ( multimap<BasicBlock*, BasicBlock* >::iterator it=CFG->bb_edges.begin() ; it != CFG->bb_edges.end(); it++ )
                        if(it->second == *block)
                        {
                            // in[b] = in[b] U (out[p])
                            unionVect((*BBtoBlockPoint)[*block]->out, (*BBtoBlockPoint)[it->first]->in);
                        }



                    // out = F_b(in[B])

                    transfer((*BBtoBlockPoint)[*block], valuesToIndex);

                    // if tmp differs from  out then we flag a modify
                    if(tmp != ((*BBtoBlockPoint)[*block]->in))
                        somethingModified = true;

                }
            }

        }

        //printProgramPoints(*(CFG->F),&BBtoBlockPoint ,&valuesToIndex);

    }

    // This function prints out the bit code with the program points drawn in.
 bool IterativeFramework::printProgramPoints( )
    {

        for(Function::BasicBlockListType::iterator bl=beginBlock; bl != endBlock; ++bl)
        {
            cout <<endl <<bl->getName().data() <<":"<<endl;

            cout << "IN: ";
            //printVector((*BBtoBlockPoint)[bl]->in);
            printValueNames(valuesToIndex, (*BBtoBlockPoint)[bl]->in);

            for(BasicBlock::InstListType::iterator inst=bl->begin(); inst != bl->end(); ++inst)
            {

                // These operations return values.
                // 0. store: Unaryinst
                // 1. binary ops
                // 2. cast ops
                // 3. PHI Nodes have values, but we don't want points infront of them.
                if(isa<PHINode>(inst)|| isa<BinaryOperator>(inst) ||
                   isa<UnaryInstruction>(inst)||isa<CmpInst>(inst))
                {

                    cout << "\t"<<(*valuesToIndex)[inst]<<" = "<< inst->getOpcodeName();
                }
                // These consume but do not produce.
                // Well actually the call could if they were pointers.
                else if(isa<TerminatorInst>(inst)||isa<StoreInst>(inst)||isa<CallInst>(inst))
                {
                    cout << "\t" << inst->getOpcodeName();
                }
                // we just spit out everything else.
                else
                {
                    cout << "\tINVALID OP"<< inst->getOpcodeName();
                }

                for(User::op_iterator oi= inst->op_begin(), oe= inst->op_end(); oi!=oe ;++oi)
                {
                    Value *v = oi->get();

                    // Do we need these values?
                    if(isa<Instruction>(v)||isa<Argument>(v))
                    {
                        if (v->hasName())
                            cout<< " " <<v->getName().data();
                        else
                            cout<< " " <<(*valuesToIndex)[v];
                    }
                }

                cout<< endl;


                // This is where the points need to be printed out:
                if(isa<BinaryOperator>(inst) ||isa<UnaryInstruction>(inst)||isa<CmpInst>(inst)||
                    isa<TerminatorInst>(inst)||isa<StoreInst>(inst)||isa<CallInst>(inst))
                {
                        printValueNames(valuesToIndex, *(*BBtoBlockPoint)[bl]->programPoints[inst]);
                        cout << endl;
                }

                if(isa<TerminatorInst>(inst))
                {
                    cout << "OUT: ";
                    printValueNames(valuesToIndex, (*BBtoBlockPoint)[bl]->out);
                }

            }
        }
    }


    // Use this to print the Control Flow Graph.
 bool IterativeFramework::printCFG()
    {
        //print out start and end
        cout<< "Start: " << CFG->start << endl;
        cout<< "End:   " << CFG->end << endl<<endl;

        //print out nodes
        cout << "Vertices:" <<endl;
        for(list<BasicBlock*>::iterator v=CFG->vertices.begin(); v !=CFG->vertices.end();++v)
        {
            cout << *v << endl;
        }



        cout<<endl<<"Edges:" <<endl;
        //print out edges
        for(multimap<BasicBlock*,BasicBlock*>::iterator it=CFG->bb_edges.begin();it!= CFG->bb_edges.end();++it)
            cout << it->first<<" -> "<< it->second <<endl;
        cout <<endl;
        return false;
    }



    cfg *CFG;
    map<BasicBlock*,blockPoints*> *BBtoBlockPoint;
    map<Value*,unsigned> *valuesToIndex;



    // This function takes in a pointer to a CFG and a pointer to an empty block map and
    // contstructs the mapping of basicblocks to blockpoints.
    //
    // Additionally, it initializes all of the vectors. So, it must keep track of
    // the number of values.
    //
    // Lastly, it produces the mapping between values and their vector index.
 bool IterativeFramework::buildBlockPointMap( cfg *CFG, map<BasicBlock*,blockPoints*> *BBtoBlockPoint,
                                                     map<Value*,unsigned> *valuesToIndex )
    {
        //map<Value*, string> name_map;
        unsigned index=0;
        int point = 0;


        // TODO: I need to iterate through the Function and make the arguments blockPoints for Start.
        blockPoints *start_bp = new blockPoints;
        blockPoints *end_bp = new blockPoints;

        (*BBtoBlockPoint)[CFG->start] = start_bp;
        (*BBtoBlockPoint)[CFG->end] = end_bp;

        for(list<BasicBlock*>::iterator block=CFG->vertices.begin(); block != CFG->vertices.end(); ++block)
        {
            BasicBlock *bl  = *block;
            blockPoints *bp = new blockPoints;

            (*BBtoBlockPoint)[bl]=bp;


            for(BasicBlock::InstListType::iterator inst=bl->begin(); inst != bl->end(); ++inst)
            {
                // These operations return values.
                // 0. store: Unaryinst
                // 1. binary ops
                // 2. cast ops
                // 3. PHI Nodes have values, but we don't want points infront of them.
                if(isa<PHINode>(inst)|| isa<BinaryOperator>(inst) ||
                   isa<UnaryInstruction>(inst)||isa<CmpInst>(inst))
                {
                    //names will become indices.
                    (*valuesToIndex)[inst]=index++;

                    // we want to flag the phi node so we don't have a point.
                    if(!isa<PHINode>(inst))
                        // we push the point on a list so we can initialize them later.
                        bp->programPoints[inst] = NULL;

                }
                // These consume but do not produce.
                // Well actually the call could if they were pointers.
                else if(isa<TerminatorInst>(inst)||isa<StoreInst>(inst)||isa<CallInst>(inst))
                {
                    bp->programPoints[inst] = NULL;
                }
                // we just spit out everything else.
                else
                {
                    cout << "\tHRM"<< inst->getOpcodeName();
                }

                for(User::op_iterator oi= inst->op_begin(), oe= inst->op_end(); oi!=oe ;++oi)
                {

                    Value *v = oi->get();

                    // Do we need these values?
                    if(isa<Instruction>(v)||isa<Argument>(v))
                    {

                        // if the argument is not in the value map then let's add it.
                        if(valuesToIndex->find(v) == valuesToIndex->end())
                            (*valuesToIndex)[v]=index++;
                    }
                }

            }

        }


        // We are going to give the arguments an index into the vector.
        for(Function::ArgumentListType::iterator ag=CFG->F->arg_begin(); ag != CFG->F->arg_end(); ++ag)
            if(valuesToIndex->find(ag) == valuesToIndex->end())
                (*valuesToIndex)[ag]=index++;

        // Note: Need to initialize start and end which means I need to grab
        //       the function arguments. This changes everything because it
        //       adds more positions to the vectors.
        start_bp->in  = bitvector(valuesToIndex->size());
        start_bp->out = bitvector(valuesToIndex->size());

        end_bp->in  = bitvector(valuesToIndex->size());
        end_bp->out  = bitvector(valuesToIndex->size());

        // we know all of the values, now we can initializes all the vectors on the heap.
        for(list<BasicBlock*>::iterator block=CFG->vertices.begin(); block != CFG->vertices.end(); ++block)
        {
            BasicBlock *bl  = *block;
            blockPoints *bp = (*BBtoBlockPoint)[bl];


            bp->in = bitvector(valuesToIndex->size());
            bp->out = bitvector(valuesToIndex->size());

            for(map<Value*, bitvector*>::iterator it = bp->programPoints.begin(); it != bp->programPoints.end(); ++it)
            {
                bp->programPoints[it->first] = new bitvector(valuesToIndex->size());
            }

            //cout << "pPS: "<< bp->programPoints.size() <<endl;
        }


    }

 bool IterativeFramework::printValueNames(map<Value*,unsigned> *valuesToIndex, bitvector& v)
    {
        bitvector *indexCalled = new bitvector(v.size());

        cout << "{ ";
        for(map<Value*,unsigned>::iterator it = valuesToIndex->begin(); it != valuesToIndex->end(); ++it)
        {
            //cout << "("<< it->first<<", "<<it->second<<"): ";
            if(v[it->second])
                if(it->first->hasName())
                    cout << " "<<it->first->getName().data();
                else if(!(*indexCalled)[it->second])
                {
                    cout << " " << it->second;
                    (*indexCalled)[it->second] = true;
                }
        }
        cout << "}"<< endl;
    }


    // Here we are trying to build the CFG of our program.
     bool IterativeFramework::buildCFG(Function &F)
    {

        map<BlockAddress*,BasicBlock*> block_addr_to_block;

        CFG->start =  BasicBlock::Create(F.getContext(),"start",NULL);
        CFG->end   =  BasicBlock::Create(F.getContext(),"end",NULL);

        // WARNING: This may be on the stack.
        // We want to keep the function in the CFG just so we can grab the arguments.
        CFG->F = &F;

        // Function > GlobalValue > Value
        string name(F.getName().data());

        // First let us build the map of BasicBlocks, and the vertex list.
        for(Function::BasicBlockListType::iterator bl=F.begin(); bl != F.end(); ++bl)
        {
            BlockAddress *ba = BlockAddress::get(bl);
            block_addr_to_block[ba]=bl;
            // WARNING: where is this list being created. The heap, right?
            CFG->vertices.push_back(bl);
        }


        for(Function::BasicBlockListType::iterator bl=F.begin(); bl != F.end(); ++bl)
        {
            BlockAddress *ba = BlockAddress::get(bl);
            TerminatorInst *t = bl-> getTerminator();
            unsigned num = t->getNumSuccessors();


            if(bl== F.begin())
            {
                pair<BasicBlock*, BasicBlock* > p = pair<BasicBlock*, BasicBlock* >(CFG->start,bl);
                CFG->bb_edges.insert(p);
            }

            if(num == 0)
            {
                pair<BasicBlock*, BasicBlock* > p = pair<BasicBlock*, BasicBlock* >(bl,CFG->end);
                CFG->bb_edges.insert(p);

            }
            else
            {
                for(int i = 0; i < num; i++)
                {
                    BasicBlock *term_bb     = t->getSuccessor(i);
                    BlockAddress *term_addr = BlockAddress::get(term_bb);

                    pair<BasicBlock*, BasicBlock* > p =
                        pair<BasicBlock*, BasicBlock* >(bl,block_addr_to_block[term_addr]);

                    CFG->bb_edges.insert(p);

                }
            }

        }

        return false;
    }



    // What follows are static funtions for bit manipulation on vectors.

    void IterativeFramework::removeElements(bitvector &vr, bitvector& v1, bitvector& v2)
    {
        for(int i=0; i < v1.size();i++)
            if(v2[i])
                vr[i]=false;
            else
                vr[i] = v1[i];
    }

    void IterativeFramework::setEmpty(bitvector& v)
    {
        for(int i=0; i < v.size();i++)
            v[i]=false;
    }

    // v1 <- v1 or v2
    void IterativeFramework::unionVect(bitvector& v1, bitvector& v2)
    {
        for(int i=0; i < v1.size();i++)
            v1[i] = v1[i] | v2[i];
    }

    void IterativeFramework::printVector(bitvector& v)
    {
        cout << "[";
        for(int i=0; i < v.size();i++)
            cout << " " <<v[i];
        cout << "]" << endl;

    }
