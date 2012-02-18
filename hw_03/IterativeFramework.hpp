#include "llvm/BasicBlock.h"
#include "llvm/Function.h"

#include <list>
#include <map>
#include <vector>

using namespace llvm;
using namespace std;

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
struct blockPoints{
    vector<bool> *in;
    vector<bool> *out;
    map<Value*, vector<bool>*> programPoints;
};


//namespace  IterativeFramework
//{

class IterativeFramework
{
public:
	IterativeFramework(Function &F);
	~IterativeFramework();

    virtual bool runIterativeFramework( bool fromTop,
                                        bool(*transfer)(blockPoints *bp, map<Value*,unsigned> *valuesToIndex)
                                        /*, meet_op, set_boundary, set_initial*/);

    virtual bool printProgramPoints();


    virtual bool printCFG();

    static void printVector(vector<bool> *v);
    static void unionVect(vector<bool> *v1,vector<bool> *v2);
    static void setEmpty(vector<bool> *v);
    static void removeElements(vector<bool> *vr,vector<bool> *v1,vector<bool> *v2);


private:
    cfg *CFG;
    map<BasicBlock*,blockPoints*> *BBtoBlockPoint;
    map<Value*,unsigned> *valuesToIndex;
    Function::BasicBlockListType::iterator endBlock;
    Function::BasicBlockListType::iterator beginBlock;

    // This function takes in a pointer to a CFG and a pointer to an empty block map and
    // contstructs the mapping of basicblocks to blockpoints.
    //
    // Additionally, it initializes all of the vectors. So, it must keep track of
    // the number of values.
    //
    // Lastly, it produces the mapping between values and their vector index.
    virtual bool buildBlockPointMap( cfg *CFG, map<BasicBlock*,blockPoints*> *BBtoBlockPoint,
                                     map<Value*,unsigned> *valuesToIndex );
    virtual bool printValueNames(map<Value*,unsigned> *valuesToIndex, vector<bool> *v);
    virtual bool buildCFG(Function &F);    // Here we are trying to build the CFG of our program.

};

//}
