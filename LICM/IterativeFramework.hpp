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

   typedef bitvector (*transfer_function)(CustomBlock&);

	IterativeFramework(Function&, direction_t, meet_operator_t, transfer_function, bitvector);
	~IterativeFramework();

   void execute(void);
   bitvector getInEntry(void) const;

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
    DataFlowGraph cfg;
    
    direction_t direction;
    meet_operator_t meet;
    
    transfer_function transfer;
    
    void buildDataFlowGraph(Function &, bitvector&);
};

//}
