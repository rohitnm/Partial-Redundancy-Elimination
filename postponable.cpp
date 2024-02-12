// ECE/CS 5544 S23: Postponable.cpp
// Group: Shambhavi Kuthe, Rohit Mehta

////////////////////////////////////////////////////////////////////////////////

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include <stdio.h>
#include <iostream>
#include "dataflow.h"
#include "available.h"
#include "postponable.h"

using namespace llvm;
using namespace std;

static char ID;
std::map<BasicBlock *, BBAttributes *> *local_available_result;


Postponable::Postponable() : FunctionPass(ID) { }

void Postponable::getAnalysisUsage(AnalysisUsage& AU) const {
        AU.setPreservesCFG();
        AU.addRequired<Available>();
} 

Postponable::PostAnalysis::PostAnalysis (int size, Domain domain, enum MeetOp mo, enum Direction dir, BitVector bv, BitVector iv) : Dataflow(size, domain, mo, dir, bv, iv) {}

BBAttributes Postponable::PostAnalysis::transferFn(BitVector in, BasicBlock* current)
{
  BBAttributes output;
  BitVector nullSet((int)domain.size(), false);
  BitVector gen = nullSet;
  BitVector kill = nullSet;
  BitVector temp1 = nullSet;
  BitVector temp2 = nullSet;
  BitVector earliest_available = nullSet;
  earliest_available = (*local_available_result)[current]->earliest;
  BitVector temp = earliest_available;

  for (BasicBlock::iterator i = current->begin(), e = current->end(); i!=e; ++i) {
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
  }

  
  output.genSet = earliest_available;         
  output.killSet = gen;                       //eUse[B]
  output.tfOut = gen;
  output.tfOut.flip();
  output.postin = in;
  temp |= in;
  output.tfOut &= temp;                       //gen;
  return output;
}

/*
void Postponable::calc_latest(std::map<BasicBlock*, BBAttributes*> *result, Function &F)
{
  BitVector nullSet((int)domain.size(), false);
  BitVector temp0 = nullSet;
  BitVector temp1 = nullSet;
  BitVector temp2 = nullSet;
  for (Function::iterator FI = F.begin(), FE = F.end(); FI != FE; ++FI)
  {
    BasicBlock* block = &*FI;
    temp0 = (*result)[block]->postin;
    temp0 |= (*result)[block]->earliest;
    for (BasicBlock *successor : successors(block))
    {
      temp1 = (*result)[successor]->earliest;
      temp1 |= (*result)[successor]->postin;
      temp2 &= temp1;
    }

    temp2 |= (*result)[block]->killSet;
    temp0 &= temp2;

  }
}*/

std::map<BasicBlock*,BBAttributes*> * Postponable::getPostponableResult()
{
  return &(postponable_result.result);
}

void Postponable::displayResultsDFA(std::map<BasicBlock*, BBAttributes*> BBAttrsMap_print, Domain domain)
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
    outs() << "Earliest[BB]: ";
    for (int j = 0; j < temp_bb_attr->genSet.size(); ++j)
    {
      if (temp_bb_attr->genSet[j])
        bb_gen_set_exp.push_back(*(Expression*)domain[j]);
    }
    printSet (&bb_gen_set_exp);

    std::vector<Expression> bb_kill_set_exp;
    outs() << "Use[BB]: ";
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

    /*
    std::vector<Expression> bb_earliest_set_exp;
    outs() << "Earliest[BB]: ";
    for (int j = 0; j < temp_bb_attr->earliest.size(); ++j)
    {
        if (temp_bb_attr->earliest[j])
            bb_earliest_set_exp.push_back(*(Expression*)domain[j]);
    }
    printSet (&bb_earliest_set_exp);*/

    outs() << "\n";
  }
}
  
bool Postponable::runOnFunction(Function& F) 
{
  available_result = getAnalysis<Available>().getAvailableResult();

  local_available_result = available_result;
  outs() << "Postponable DFA\n";
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
  //Postponable expressions : Meet operator = Intersection, Direction = Forward
  PostAnalysis analysis(domain.size(), domain, INTERSECTION, FORWARD, boundary, init);
  //Run dataflow analysis and store result 
  postponable_result = analysis.dataflowAnalysis(F);
  //Display analysis results
  displayResultsDFA(postponable_result.result, domain);

  // Did not modify the incoming Function.
  return false;
}
  // void PostponableExpressions::getAnalysisUsage(AnalysisUsage& AU) const {
  //   AU.setPreservesAll();
  // }  

char Postponable::ID = 2;
RegisterPass<Postponable> postponable("Postponable", "ECE/CS 5544 Postponable Expressions");