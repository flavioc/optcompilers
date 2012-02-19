#include "llvm/BasicBlock.h"
#include "llvm/Function.h"

#include <list>
#include <map>
#include <vector>

using namespace llvm;
using namespace std;

typedef std::vector<bool> bitvector;

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

//namespace  IterativeFramework
//{

class IterativeFramework
{
public:

   typedef enum {
      DIRECTION_FORWARD,
      DIRECTION_BACKWARD
   } direction_t;

   typedef enum {
      BLOCK_BLOCK,
      BLOCK_INSTRUCTION
   } graph_type_t;

   typedef bool (*transfer_function)(blockPoints *, map<Value*, unsigned> *);

	IterativeFramework(Function &F, graph_type_t type, direction_t dir);
	~IterativeFramework();

    virtual bool runIterativeFramework(transfer_function transfer /*, meet_op, set_boundary, set_initial*/);

    virtual bool printProgramPoints();


    virtual bool printCFG();

    static void printVector(bitvector &);
    static void unionVect(bitvector&, bitvector&);
    static void setEmpty(bitvector&);
    static void removeElements(bitvector&, bitvector&, bitvector&);


private:
    cfg *CFG;
    map<BasicBlock*,blockPoints*> *BBtoBlockPoint;
    map<Value*,unsigned> *valuesToIndex;
    Function::BasicBlockListType::iterator endBlock;
    Function::BasicBlockListType::iterator beginBlock;
    direction_t direction;
    graph_type_t graph;

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

};

//}
