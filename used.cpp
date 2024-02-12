// ECE/CS 5544 S23 Used.cpp
// Group: Shambhavi Kuthe, Rohit Mehta

////////////////////////////////////////////////////////////////////////////////

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include <stdio.h>
#include <iostream>
#include "dataflow.h"
#include "used.h"
#include "postponable.h"


using namespace llvm;
using namespace std;

static char ID;
std::map<BasicBlock *, BBAttributes *> *local_postponable_result;

BitVector calc_latest(BasicBlock *block, std::map<BasicBlock*, BBAttributes*> *local_postponable_result, int domainSize);

Used::Used() : FunctionPass(ID) { }

void Used::getAnalysisUsage(AnalysisUsage& AU) const {
  AU.setPreservesAll();
  AU.addRequired<Postponable>();
}

Used::UsedAnalysis::UsedAnalysis (int size, Domain domain, enum MeetOp mo, enum Direction dir, BitVector bv, BitVector iv) : Dataflow(size, domain, mo, dir, bv, iv) {}

BBAttributes Used::UsedAnalysis::transferFn(BitVector in, BasicBlock* current)
{
  BBAttributes output;
  BitVector nullSet((int)domain.size(), false);
  BitVector gen = nullSet;
  BitVector temp = in;

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
  
  output.genSet = gen;     //eUse[B]
  output.tfOut = calc_latest(current, local_postponable_result, domain.size());
  output.latest = calc_latest(current, local_postponable_result, domain.size());
  output.tfOut.flip();
  temp |= gen;
  output.tfOut &= temp;    //gen;
  return output;
}

BitVector calc_latest(BasicBlock *block, std::map<BasicBlock*, BBAttributes*> *local_postponable_result, int domainSize)
{
    BitVector nullSet(domainSize, false);
    BitVector fullSet(domainSize, true);
    BitVector temp0 = nullSet;
    BitVector temp1 = nullSet;
    BitVector temp2 = fullSet;
    temp0 = (*local_postponable_result)[block]->postin;
    temp0 |= (*local_postponable_result)[block]->earliest;
    for (BasicBlock *successor : successors(block))
    {
        temp1 = (*local_postponable_result)[successor]->earliest;
        temp1 |= (*local_postponable_result)[successor]->postin;
        temp2 &= temp1;
    }
    temp2 = temp2.flip();
    temp2 |= (*local_postponable_result)[block]->killSet;               // Kill set from Posponable is eUse[B], not to be confused with original Kill set
    temp0 &= temp2;

    return temp0;
}

std::map<BasicBlock*,BBAttributes*> * Used::getUsedResult()
{
  return &(used_result.result);
}

Domain *Used::getUsedDomain()
{
  return &domain;
}


      //Function to print the dataflow analysis results
void Used::displayResultsDFA(std::map<BasicBlock*, BBAttributes*> BBAttrsMap_print, Domain domain)
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

    std::vector<Expression> bb_latest_set_exp;
    outs() << "latest[BB]: ";
    for (int j = 0; j < temp_bb_attr->latest.size(); ++j)
    {
      if (temp_bb_attr->latest[j])
        bb_latest_set_exp.push_back(*(Expression*)domain[j]);
    }
    printSet (&bb_latest_set_exp);

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
      
bool Used::runOnFunction(Function& F) 
{
  postponable_result = getAnalysis<Postponable>().getPostponableResult();

  local_postponable_result = postponable_result;
  outs() << "UsedDFA\n";
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
UsedAnalysis analysis(domain.size(), domain, UNION, BACKWARD, boundary, init);
//Run dataflow analysis and store result 
used_result = analysis.dataflowAnalysis(F);
  //Display analysis results
displayResultsDFA(used_result.result, domain);

// Did not modify the incoming Function.
return false;
}
  
char Used::ID = 3;
RegisterPass<Used> used("Used", "ECE/CS 5544 Used Expressions");
