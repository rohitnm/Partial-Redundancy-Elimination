// ECE/CS 5544 S23 lcm.cpp
// Group: Shambhavi Kuthe, Rohit Mehta

////////////////////////////////////////////////////////////////////////////////

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include "dataflow.h"
#include "used.h"

using namespace llvm;
using namespace std;


namespace
{
    class lcmpass: public FunctionPass
    {
        public:
            static char ID;
            Domain domain;

            lcmpass() : FunctionPass(ID) {}

            virtual void getAnalysisUsage(AnalysisUsage &AU) const
            {
                AU.setPreservesCFG();
                AU.addRequired<Used>();
            }

            //Maps domain to an integer for getting map index
            int domainIndexNew(void* ptr)
            {
                Domain &D = this->domain;
                auto it = std::find(D.begin(), D.end(), ptr);
                int i = std::distance(D.begin(), it);
                if (i >= this->domain.size() || i < 0) {
                    i = -1;
                }
                return i;
            }

            virtual bool runOnFunction(Function& F)
            {
                outs() << "LCM Pass\n";
                outs() << "Function name: " << F.getName() << "\n";

                // Domain domain;
                vector <Expression> expressions;
                std::vector<Instruction *>  instrList;

                std::map<BasicBlock*, BBAttributes*> *used_result;
                used_result = getAnalysis<Used>().getUsedResult();

                std::map <Expression *, AllocaInst*> exp_alloca_map;
                BasicBlock::iterator functionStartInstruction = F.getEntryBlock().getFirstInsertionPt();
                
                
                for (Function::iterator FI = F.begin(), FE = F.end(); FI != FE; ++FI) {
                    BasicBlock* block = &*FI;

                    for (BasicBlock::iterator i = block->begin(), e = block->end(); i!=e; ++i) {           
                    Instruction* I = &*i;

                    if (BinaryOperator *BI = dyn_cast<BinaryOperator>(I)) {
                        Expression *expr = new Expression(BI); 
                        bool found = false;

                        for(void* entry : domain)
                        {
                            if((*expr) == *((Expression *)entry))
                            {
                                found = true;
                                break;
                            }
                        }

                        if(found == false)
                        {
                            domain.push_back(expr);
                            instrList.push_back(BI);
                        }
                        else
                        {
                            delete expr;
                        }
                    
                    }
                    }
                }

                for (auto e : domain)
                {
                    exp_alloca_map[(Expression*)e] = NULL;
                }

                for (auto e : domain)
                {
                    for (Function::iterator FI = F.begin(), FE = F.end(); FI != FE; ++FI)
                    {                                

                        BasicBlock *BB = &*FI;

                        BitVector usedNlatest((int)domain.size(), false);
                        usedNlatest = (*used_result)[BB]->latest;
                        usedNlatest &= (*used_result)[BB]->output;

                        if(usedNlatest[domainIndexNew(e)])
                        {
                            if (BinaryOperator *BI = dyn_cast < BinaryOperator > (BB->getFirstInsertionPt()))
                            {
                                exp_alloca_map[(Expression*)e] = new AllocaInst(BI->getType(), 0, "tempPtr", &*functionStartInstruction);
                                break;
                            }
                        }
                    }
                }

                
                std::vector<llvm::Value *>  clonedList(domain.size(), nullptr); 
            

                std::vector<Expression> bb_out_set_exp;
                outs() << "Domain: ";
                for (auto j = 0; j < domain.size(); ++j)
                {
                    bb_out_set_exp.push_back(*(Expression*)domain[j]);
                }
                printSet (&bb_out_set_exp);
                outs() << "\n";

                //Inserting temporary variables
                for (Function::iterator FI = F.begin(), FE = F.end(); FI != FE; ++FI) 
                {
                    BasicBlock* BB = &*FI;

                    BitVector usedNlatest((int)domain.size(), false);
                    usedNlatest = (*used_result)[BB]->latest;
                    usedNlatest &= (*used_result)[BB]->output;
                    // outs() << "For t ";
                    // printBitVector(usedNlatest);

                    BitVector tempReplaceExpr((int)domain.size(), false);
                    BitVector temp((int)domain.size(), false);
                    tempReplaceExpr = (*used_result)[BB]->genSet; //genSet of used has e_use stored
                    temp = (*used_result)[BB]->latest;
                    temp.flip();
                    temp |= (*used_result)[BB]->output;
                    tempReplaceExpr &= temp;
                    // outs() << "For Replacing ";
                    // printBitVector(tempReplaceExpr);

                    BitVector toBeReplacedExpr((int)domain.size(), false);
                    toBeReplacedExpr = usedNlatest;
                    toBeReplacedExpr &= tempReplaceExpr;

                    // outs() << "For Replacing ";
                    // printBitVector(toBeReplacedExpr);
                    
                    bool cloned = false;
                    bool replaced = false;

                    //Inserting temporary variables
                    for (auto I : instrList) 
                    {      
                        if (auto *BinOp = dyn_cast<BinaryOperator>(I))
                        {
                            Expression *expr = new Expression(BinOp); 
                            bool found = false;
                            int i = 0;
                            for(void* entry : domain)
                            {
                                if((*expr) == *((Expression *)entry))
                                {
                                    found = true;
                                    i++;
                                    break;
                                }
                                i++;
                            }

 
                            if(usedNlatest[i-1] && cloned == false && found == true)
                            {
                                Instruction *new_inst = BinaryOperator::Create(BinOp->getOpcode(),BinOp->getOperand(0), BinOp->getOperand(1),"t", BB->getTerminator());
                                Value* new_val = dyn_cast<Value>(new_inst);
                                StoreInst *str_inst = new StoreInst(new_inst, exp_alloca_map[(Expression*)domain[i-1]], BB->getTerminator());
                                outs() << "BB Name: " << BB->getName() << "\n";
                                outs()<<"  Instr  \t"<<(*new_inst)<<"\n";
                                outs()<<"  Store  \t"<<(*str_inst)<<"\n";
                                clonedList[i-1] = new_val;
                                cloned = true;

                            }  
                        }

                    }
                    
                }

                //Replacing all the variables
                for (Function::iterator FI = F.begin(), FE = F.end(); FI != FE; ++FI) 
                {
                    BasicBlock* BB = &*FI;
                    bool replaced = false;

                    BitVector usedNlatest((int)domain.size(), false);
                    usedNlatest = (*used_result)[BB]->latest;
                    usedNlatest &= (*used_result)[BB]->output;
                    // outs() << "For t ";
                    // printBitVector(usedNlatest);

                    BitVector tempReplaceExpr((int)domain.size(), false);
                    BitVector temp((int)domain.size(), false);
                    tempReplaceExpr = (*used_result)[BB]->genSet; //genSet of used has e_use stored
                    temp = (*used_result)[BB]->latest;
                    temp.flip();
                    temp |= (*used_result)[BB]->output;
                    tempReplaceExpr &= temp;
                    // outs() << "For Replacing ";
                    // printBitVector(tempReplaceExpr);

                    BitVector toBeReplacedExpr((int)domain.size(), false);
                    toBeReplacedExpr = usedNlatest;
                    toBeReplacedExpr &= tempReplaceExpr;


                    for (auto I : instrList) 
                    {                        
                        if (auto *BinOp = dyn_cast<BinaryOperator>(I))
                        {
                            Expression *expr = new Expression(BinOp); 
                            bool found = false;
                            int i = 0;
                            for(void* entry : domain)
                            {
                                if((*expr) == *((Expression *)entry))
                                {
                                    found = true;
                                    i++;
                                    break;
                                }
                                i++;
                            }
                            if(toBeReplacedExpr[i-1] && found == true && replaced == false)
                            {
                                BasicBlock::iterator nit(I);
                                ReplaceInstWithValue(BB->getInstList(), nit, clonedList[i-1] );
                                LoadInst *load_inst = new LoadInst(BinOp->getType(), exp_alloca_map[(Expression*)domain[i-1]], "loadValue", BB->getTerminator());
                                outs() << "BB Name: " << BB->getName() << "\n";
                                outs()<<"  Load  \t"<<(*load_inst)<<"\n";
                                replaced = true;
                                outs() << "Replaced............\n";
                            }
                        }
                    }
                }

                           
        return false;
    }
};
char lcmpass::ID = 4;
RegisterPass<lcmpass> lcmpass("lazy-code-motion", "ECE/CS 5544 Lazy code motion");  
}