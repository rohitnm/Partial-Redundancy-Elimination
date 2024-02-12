#ifndef __ANTICIPATED_H__
#define __ANTICIPATED_H__

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "dataflow.h"

using namespace llvm;
using namespace std;

class Anticipated : public FunctionPass 
{
    private:
    DFAResult anticipated_result;

    void displayResultsDFA(std::map<BasicBlock*, BBAttributes*> BBAttrsMap_print, Domain domain);

    public:
    static char ID;
    Domain domain;
    Anticipated();
    virtual bool runOnFunction(Function &F);

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

    std::map<BasicBlock*,BBAttributes*> *getAnticipatedResult();

    class AntiAnalysis : public Dataflow 
    {
    public:
        AntiAnalysis (int size, Domain domain, enum MeetOp mo, enum Direction dir, BitVector bv, BitVector iv);
        BBAttributes transferFn (BitVector in, BasicBlock* current);
    };
       
};
#endif
