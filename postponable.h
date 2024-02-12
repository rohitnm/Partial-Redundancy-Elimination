#ifndef __POSTPONABLE_H__
#define __POSTPONABLE_H__

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "dataflow.h"

using namespace llvm;
using namespace std;


class Postponable : public FunctionPass 
{

    private:
    DFAResult postponable_result;
    void displayResultsDFA(std::map<BasicBlock*, BBAttributes*> BBAttrsMap_print, Domain domain);

    public:
    static char ID;
    std::map<BasicBlock *, BBAttributes *> *available_result;
    Domain domain;
    Postponable();
    virtual bool runOnFunction(Function &F);

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

    std::map<BasicBlock*,BBAttributes*> *getPostponableResult();

    class PostAnalysis : public Dataflow 
    {
    public:

        PostAnalysis (int size, Domain domain, enum MeetOp mo, enum Direction dir, BitVector bv, BitVector iv);
        BBAttributes transferFn (BitVector in, BasicBlock* current);
    };
       
};
#endif
