// ECE/CS 5544 S23 anticipated.cpp
// Group: Shambhavi Kuthe, Rohit Mehta

////////////////////////////////////////////////////////////////////////////////

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include <stdio.h>
#include <iostream>
#include "dataflow.h"
#include "anticipated.h"


using namespace llvm;
using namespace std;

static char ID;

Anticipated::Anticipated() : FunctionPass(ID) { }

void Anticipated::getAnalysisUsage(AnalysisUsage& AU) const {
  AU.setPreservesAll();
}

Anticipated::AntiAnalysis::AntiAnalysis (int size, Domain domain, enum MeetOp mo, enum Direction dir, BitVector bv, BitVector iv) : Dataflow(size, domain, mo, dir, bv, iv) {}

BBAttributes Anticipated::AntiAnalysis::transferFn(BitVector in, BasicBlock* current)
{
  BBAttributes output;
  BitVector nullSet((int)domain.size(), false);
  BitVector gen = nullSet;
  BitVector kill = nullSet;

  for (BasicBlock::iterator i = current->begin(), e = current->end(); i!=e; ++i) 
  {
    Instruction * I = &*i;
    //Gen set
    if (BinaryOperator * BI = dyn_cast<BinaryOperator>(I)) 
    {
      Expression *expr = new Expression(BI);
      Expression *foundExpr = NULL;
      bool found = false;

      for(void* entry : domain)
      {
        if((*expr) == *((Expression *) entry))
        {
          found = true;
          foundExpr = (Expression *) entry;
          break;
        }
      }

      if(found && (domainIndex((void*)foundExpr)<domain.size()))
      {
        gen.set(domainIndex((void*)foundExpr));
      }
    
    }
    StringRef inst  =  I->getName();
    if(!inst.empty()){
      for(auto itr = domain.begin() ; itr != domain.end() ; itr++)
      {
        Expression* expr = (Expression*) (*itr);

        StringRef op1 = expr->v1->getName();
        StringRef op2 = expr->v2->getName();

        //If either of the operands is same, expression is killed
        if(op1.equals(inst) || op2.equals(inst))
        {
          int idx = domainIndex((void*)expr);
          if(idx < domain.size()){
            kill.set(idx);
            gen.reset(idx);
          }
        }
      }
    }
  }

output.genSet = gen;
output.killSet = kill;
output.tfOut = kill;
output.tfOut.flip();
output.tfOut &= in;
output.tfOut |= gen;

return output;

}

std::map<BasicBlock*,BBAttributes*> * Anticipated::getAnticipatedResult()
{
  return &(anticipated_result.result);
}


      //Function to print the dataflow analysis results
void Anticipated::displayResultsDFA(std::map<BasicBlock*, BBAttributes*> BBAttrsMap_print, Domain domain)
{    
  std::map<BasicBlock*, BBAttributes*>::iterator i;

  for (i = BBAttrsMap_print.begin(); i != BBAttrsMap_print.end(); ++i)
  {

    struct BBAttributes *temp_bb_attr = BBAttrsMap_print[i->first];
    if (temp_bb_attr->BB->hasName())
        outs() << "BB Name: " << temp_bb_attr->BB->getName() << "\n";
    else
    {
        outs() << "BB Name: ";
        temp_bb_attr->BB->printAsOperand(outs(), false);
        outs() << "\n";
    }

    std::vector<Expression> bb_gen_set_exp;
    //Domain bb_gen_set_exp;
    outs() << "gen[BB]: ";
    for (int j = 0; j < temp_bb_attr->genSet.size(); ++j)
    {
      if (temp_bb_attr->genSet[j])
        bb_gen_set_exp.push_back(*(Expression*)domain[j]);
    }
    printSet (&bb_gen_set_exp);

    std::vector<Expression> bb_kill_set_exp;
    outs() << "kill[BB]: ";
    for (int j = 0; j < temp_bb_attr->killSet.size(); ++j)
    {
      if (temp_bb_attr->killSet[j])
        bb_kill_set_exp.push_back(*(Expression*)domain[j]);
    }
    printSet (&bb_kill_set_exp);

    std::vector<Expression> bb_in_set_exp;
    outs() << "IN[BB]: ";
    for (int j = 0; j < temp_bb_attr->input.size(); ++j)
    {
        if (temp_bb_attr->input[j])
            bb_in_set_exp.push_back(*(Expression*)domain[j]);
    }
    printSet (&bb_in_set_exp);

    std::vector<Expression> bb_out_set_exp;
    outs() << "OUT[BB]: ";
    for (int j = 0; j < temp_bb_attr->output.size(); ++j)
    {
        if (temp_bb_attr->output[j])
            bb_out_set_exp.push_back(*(Expression*)domain[j]);
    }
    printSet (&bb_out_set_exp);
    outs() << "\n";
  }
}
      
bool Anticipated::runOnFunction(Function& F) 
{
  
  outs() << "Anticipated DFA\n";
  outs() << "Function name: " << F.getName() << "\n";
  // Here's some code to familarize you with the Expression
  // class and pretty printing code we've provided:
  Domain domain;
  vector <Expression> expressions;
  
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
          domain.push_back(expr);
        else
          delete expr;
      
      }
    }
  }
  
// Print out the expressions used in the function
// outs() << "Expressions used by this function:\n";
// printSet(&expressions);
outs() << "DOMAIN set: \n";
//printSetInstr (&domain);

//Initializing boundary and init vectors
BitVector boundary(domain.size(), false);
BitVector init(domain.size(), false);
//Pass set of expressions(domain), size of domain, meet operator, direction, boundary and initial conditions to dataflow framework 
//Available expressions : Meet operator = Intersection, Direction = Forward
AntiAnalysis analysis(domain.size(), domain, INTERSECTION, BACKWARD, boundary, init);
//Run dataflow analysis and store result 
anticipated_result = analysis.dataflowAnalysis(F);
  //Display analysis results
displayResultsDFA(anticipated_result.result, domain);

// Did not modify the incoming Function.
return false;
}
  
char Anticipated::ID = 0;
RegisterPass<Anticipated> X("anticipated", "ECE/CS 5544 Anticipated Expressions");
