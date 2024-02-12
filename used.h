#ifndef __USED_H__
#define __USED_H__

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "dataflow.h"

using namespace llvm;
using namespace std;

class Used : public FunctionPass 
{
    private:
    DFAResult used_result;

    void displayResultsDFA(std::map<BasicBlock*, BBAttributes*> BBAttrsMap_print, Domain domain);

    public:
    static char ID;
    std::map<BasicBlock *, BBAttributes *> *postponable_result;
    Domain domain;
    //BitVector calc_latest(BasicBlock *block, std::map<BasicBlock*, BBAttributes*> *local_postponable_result, int domainSize);
    Used();
    virtual bool runOnFunction(Function &F);

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

    std::map<BasicBlock*,BBAttributes*> *getUsedResult();
    Domain *getUsedDomain();

    class UsedAnalysis : public Dataflow 
    {
    public:
        UsedAnalysis (int size, Domain domain, enum MeetOp mo, enum Direction dir, BitVector bv, BitVector iv);
        BBAttributes transferFn (BitVector in, BasicBlock* current);
    };
       
};
#endif
