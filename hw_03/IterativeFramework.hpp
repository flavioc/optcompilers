#include "llvm/BasicBlock.h"
#include "llvm/Function.h"

#include <list>
#include <map>
#include <vector>

using namespace llvm;
using namespace std;

typedef std::vector<bool> bitvector;

struct CustomBlock {
   BasicBlock *blk;

   bitvector in;
   bitvector out;

   bool first_time;
   bool in_queue;
};

typedef map<BasicBlock*, CustomBlock*> mapping_map;

struct DataFlowGraph {
   list<CustomBlock*> nodes;
   multimap<CustomBlock *, CustomBlock *> edges;
   mapping_map mapping;
   CustomBlock *start;
   CustomBlock *end;
};

struct cfg {
    list<BasicBlock*> vertices;
    multimap<BasicBlock*, BasicBlock* > bb_edges;
    BasicBlock *start;
    BasicBlock *end;

    // WARNING: This really isn't the right place for this, but we have to
    //          pass the arguments somehow.
    Function *F;

};

// At every point and IN/OUT there is a vector corresponding to all values
// used throughout the program. Depending on the analysis being run the
// values are flagged accordingly.
//
// with this said there also, must be a map between Values->index
//
// Additionally, program points are mapped by the instruction after them.
struct blockPoints {
    bitvector in;
    bitvector out;
    map<Value*, bitvector*> programPoints;
};

class IterativeFramework
{
public:

   typedef enum {
      DIRECTION_FORWARD,
      DIRECTION_BACKWARD
   } direction_t;

   typedef enum {
      MEET_UNION,
      MEET_INTERSECTION
   } meet_operator_t;

   typedef bitvector (*transfer_function2)(CustomBlock&);
   typedef bool (*transfer_function)(blockPoints *, map<Value*, unsigned> *);

	IterativeFramework(Function&, direction_t, meet_operator_t, transfer_function2, bitvector);
	~IterativeFramework();

   void execute(void);
   bitvector getInEntry(void) const;

    virtual bool runIterativeFramework(transfer_function transfer /*, meet_op, set_boundary, set_initial*/);

    virtual bool printProgramPoints();

    virtual bool printCFG();

    static bitvector doMeetWithOperator(meet_operator_t, bitvector&, bitvector&);
    static void printVector(bitvector&);
    static bitvector unionVect(bitvector&, bitvector&);
    static bitvector intersectVect(bitvector&, bitvector&);
    static void setEmpty(bitvector&);
    static void removeElements(bitvector&, bitvector&, bitvector&);

    CustomBlock *getMap(BasicBlock *);
    const CustomBlock *getMap(BasicBlock *) const;

private:

    Function *fun;
    DataFlowGraph cfg2;
    cfg *CFG;
    map<BasicBlock*,blockPoints*> *BBtoBlockPoint;
    map<Value*,unsigned> *valuesToIndex;
    Function::BasicBlockListType::iterator endBlock;
    Function::BasicBlockListType::iterator beginBlock;
    direction_t direction;
    meet_operator_t meet;
    transfer_function transfer;
    transfer_function2 transfer2;

    // This function takes in a pointer to a CFG and a pointer to an empty block map and
    // contstructs the mapping of basicblocks to blockpoints.
    //
    // Additionally, it initializes all of the vectors. So, it must keep track of
    // the number of values.
    //
    // Lastly, it produces the mapping between values and their vector index.
    virtual bool buildBlockPointMap( cfg *CFG, map<BasicBlock*,blockPoints*> *BBtoBlockPoint,
                                     map<Value*,unsigned> *valuesToIndex );
    virtual bool printValueNames(map<Value*,unsigned> *valuesToIndex, bitvector&);
    virtual bool buildCFG(Function &F);    // Here we are trying to build the CFG of our program.

    void buildDataFlowGraph(Function &, bitvector&);
};

//}
