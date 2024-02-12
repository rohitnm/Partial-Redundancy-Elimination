
////////////////////////////////////////////////////////////////////////////////

#ifndef __CLASSICAL_DATAFLOW_H__
#define __CLASSICAL_DATAFLOW_H__

#include <stdio.h>
#include <iostream>
#include <queue>
#include <vector>

#include "llvm/IR/Instructions.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/IR/CFG.h"

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/ADT/PostOrderIterator.h"
#include <algorithm>
#include <stack>
#include <map>
#include <string>

namespace llvm {

// Add definitions (and code, depending on your strategy) for your dataflow
// abstraction here.

class Expression {
public:
    Value * v1;
    Value * v2;
    Instruction::BinaryOps op;
    Expression (Instruction * I);
    Expression ();
    bool operator== (const Expression &e2) const;
    bool operator< (const Expression &e2) const;
    std::string toString() const;
    Instruction *instr; //to store instruction for cloning
};


enum Direction{FORWARD, BACKWARD}; //FOWRWARD = 0, BACKWARD = 1
enum MeetOp{UNION, INTERSECTION}; // UNION = 0, INTERSECTION = 1
enum BBType{ENTRY, EXIT, MIDDLE}; //ENTRY=0, EXIT=1, MIDDLE=2

typedef std::vector<void*> Domain;

struct BBAttributes{
    enum BBType BBType;
    BasicBlock* BB;
    BitVector input;
    BitVector output;
    BitVector genSet;
    BitVector killSet;
    BitVector tfOut;
    BitVector earliest;
    BitVector latest;
    BitVector postin;
    std::vector<BasicBlock*> predBB;
    std::vector<BasicBlock*> succBB;
};

struct DFAResult{

    std::map<BasicBlock*,BBAttributes*> result;
};


class Dataflow{
    private:
        int size;
        BitVector boundary;
        BitVector init;
        enum MeetOp meetOp;
        enum Direction direction;
        std::vector<BasicBlock*> forwardInstructions;
        std::vector<BasicBlock*> backwardInstructions;

    public:
  
        Domain domain;
	    std::vector<std::string> domainS;
        std::map<BasicBlock*,BBAttributes*> BBAttrsMap;
        std::map<BasicBlock*, std::vector<llvm::BitVector>*> BBInstrMap;
        //Constructor to initialize all the  variables
        Dataflow(int size, Domain d, enum MeetOp mo, enum Direction dir, BitVector bv, BitVector iv)
        {
            size = size;
            domain = d;
            meetOp = mo;
            direction = dir;
            boundary = bv;
            init = iv;
        }

        void BBinit(Function &F);
        void flowOrder(Function &F);
        void extractPredSuccBB(BasicBlock* BB, struct BBAttributes* BBAttr);
        BitVector meetFn(BasicBlock* BB);
        int domainIndex(void* ptr);
        virtual BBAttributes transferFn(BitVector in, BasicBlock* current) = 0;
        DFAResult dataflowAnalysis(Function &F);
        void printMap();
};

    void printSet(std::vector<Expression> * x);
    void printSetValue(std::vector<void *> * x);
    void printSetBB(std::vector<void *> * x); 
    void printSetInstr(std::vector<void *> * x);
    void printString(std::vector<std::string > * x);
    void printBitVector(BitVector bv);
    void printDomain(Domain d);

}

#endif
