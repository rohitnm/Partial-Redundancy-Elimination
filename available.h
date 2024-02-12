#ifndef __AVAILABLE_H__
#define __AVAILABLE_H__

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "dataflow.h"

using namespace llvm;
using namespace std;


class Available : public FunctionPass 
{

    private:
    DFAResult available_result;
    void displayResultsDFA(std::map<BasicBlock*, BBAttributes*> BBAttrsMap_print, Domain domain);

    public:
    static char ID;
    std::map<BasicBlock *, BBAttributes *> *anticipated_result;
    Domain domain;
    Available();
    virtual bool runOnFunction(Function &F);

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

    std::map<BasicBlock*,BBAttributes*> *getAvailableResult();

    class AvailAnalysis : public Dataflow 
    {
    public:

        AvailAnalysis (int size, Domain domain, enum MeetOp mo, enum Direction dir, BitVector bv, BitVector iv);
        BBAttributes transferFn (BitVector in, BasicBlock* current);
    };
       
};
#endif
