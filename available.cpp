// ECE/CS 5544 S23 available.cpp
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
#include "available.h"

using namespace llvm;
using namespace std;

static char ID;
std::map<BasicBlock *, BBAttributes *> *local_anticipated_result;


Available::Available() : FunctionPass(ID) { }

void Available::getAnalysisUsage(AnalysisUsage& AU) const {
        // AU.setPreservesAll();
        AU.setPreservesCFG();
        AU.addRequired<Anticipated>();
} 

Available::AvailAnalysis::AvailAnalysis (int size, Domain domain, enum MeetOp mo, enum Direction dir, BitVector bv, BitVector iv) : Dataflow(size, domain, mo, dir, bv, iv) {}

BBAttributes Available::AvailAnalysis::transferFn(BitVector in, BasicBlock* current)
{
  BBAttributes output;
  BitVector nullSet((int)domain.size(), false);
  // BitVector gen = nullSet;
  BitVector kill = nullSet;
  BitVector antiInput = (*local_anticipated_result)[current]->input;
  BitVector temp = antiInput;

  for (BasicBlock::iterator i = current->begin(), e = current->end(); i!=e; ++i) {
    Instruction * I = &*i;
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
            // gen.reset(idx);
          }
        }
      }
    }
  }

  output.genSet = antiInput;//gen;
  output.killSet = kill;
  output.tfOut = kill;
  output.tfOut.flip();
  temp |= in;
  output.tfOut &= temp;//gen;
  output.earliest = in;
  output.earliest.flip();
  output.earliest &= antiInput;

  return output;

}

std::map<BasicBlock*,BBAttributes*> * Available::getAvailableResult()
{
  return &(available_result.result);
}

void Available::displayResultsDFA(std::map<BasicBlock*, BBAttributes*> BBAttrsMap_print, Domain domain)
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

    std::vector<Expression> bb_earliest_set_exp;
    outs() << "Earliest[BB]: ";
    for (int j = 0; j < temp_bb_attr->earliest.size(); ++j)
    {
        if (temp_bb_attr->earliest[j])
            bb_earliest_set_exp.push_back(*(Expression*)domain[j]);
    }
    printSet (&bb_earliest_set_exp);

    outs() << "\n";
  }
}
  
bool Available::runOnFunction(Function& F) 
{
  anticipated_result = getAnalysis<Anticipated>().getAnticipatedResult();

  local_anticipated_result = anticipated_result;

  outs() << "Available DFA\n";
  outs() << "Function name: " << F.getName() << "\n";
  // Here's some code to familarize you with the Expression
  // class and pretty printing code we've provided:
  Domain domain;
  vector <Expression> expressions;
  
  for (Function::iterator FI = F.begin(), FE = F.end(); FI != FE; ++FI) {
    BasicBlock* block = &*FI;

    // antiInput = (*anticipated_result)[block]->input;

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
  AvailAnalysis analysis(domain.size(), domain, INTERSECTION, FORWARD, boundary, init);
  //Run dataflow analysis and store result 
  available_result = analysis.dataflowAnalysis(F);
  //Display analysis results
  displayResultsDFA(*anticipated_result, domain);
  displayResultsDFA(available_result.result, domain);

  // Did not modify the incoming Function.
  return false;
}
  // void AvailableExpressions::getAnalysisUsage(AnalysisUsage& AU) const {
  //   AU.setPreservesAll();
  // }  

char Available::ID = 1;
RegisterPass<Available> available("available", "ECE/CS 5544 Available Expressions");