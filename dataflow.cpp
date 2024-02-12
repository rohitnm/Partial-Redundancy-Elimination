//ECE5544: COMPILER OPTIMIZATION
//Group : Rohit Mehta, Shambhavi Kuthe
////////////////////////////////////////////////////////////////////////////////

#include "dataflow.h"

namespace llvm {

  // Add code for your dataflow abstraction here.

  //Maps domain to an integer for getting map index
  int Dataflow::domainIndex(void* ptr)
  {
    Domain &D = this->domain;
    auto it = std::find(D.begin(), D.end(), ptr);
    int i = std::distance(D.begin(), it);
    if (i >= this->domain.size() || i < 0) {
        i = -1;
    }
    return i;
  }

//  Index Dataflow::instIndex(void *ptr);
  //Meet function : Stores output/input of predecessor/successor basic blocks in meetVectors and then applies the apt meet operator
  //Takes basicblock as parameter and stores result in output/input of that basic block depending on the propagation direction
  //OR operator for UNION, AND operator for INTERSECTION
  BitVector Dataflow::meetFn(BasicBlock* BB)
  {
    std::vector<BitVector> meetVectors;
    BitVector backwardMeetVectors;
    //For Forward propagation
    if(direction == FORWARD){
      for(auto i = BBAttrsMap[BB]->predBB.begin(); i != BBAttrsMap[BB]->predBB.end(); ++i){
        meetVectors.push_back(BBAttrsMap[*i]->output);
      }
      
      if (!meetVectors.empty())
      {
          backwardMeetVectors = meetVectors[0];
          for (int i = 1; i < (int) meetVectors.size(); ++i)
          {
              if (meetOp == UNION)
              {
                  backwardMeetVectors |= meetVectors[i];
              }
              else if (meetOp == INTERSECTION)
              {
                  backwardMeetVectors &= meetVectors[i];
              }
          }
          BBAttrsMap[BB]->input = backwardMeetVectors;
      }

    }
    //For Backward propagation
    else if(direction == BACKWARD){
      for (auto i = BBAttrsMap[BB]->succBB.begin(); i != BBAttrsMap[BB]->succBB.end(); ++i){
        meetVectors.push_back(BBAttrsMap[*i]->input);
      }
      if (!meetVectors.empty())
      {
          backwardMeetVectors = meetVectors[0];
          for (int i = 1; i < (int) meetVectors.size(); ++i)
          {
              if (meetOp == UNION)
              {
                  backwardMeetVectors |= meetVectors[i];
              }
              else if (meetOp == INTERSECTION)
              {
                  backwardMeetVectors &= meetVectors[i];
              }
          }
          BBAttrsMap[BB]->output = backwardMeetVectors;
      }
    }
    
    return backwardMeetVectors;
  }

  //Extracts Predecessor and Successor basic blocks of a given block and stores them in predBB & succBB
  void Dataflow::extractPredSuccBB(BasicBlock* BB, struct BBAttributes* BBAttr){
    for (BasicBlock *predecessor : predecessors(BB))
    {
      BBAttr->predBB.push_back(predecessor);
    }

    for (BasicBlock *successor : successors(BB))
    {
      BBAttr->succBB.push_back(successor);
    }
  }

  //Initializes the attributes of basic blocks to empty, gets predecessors and successors and the type of basicblock
  void Dataflow::BBinit(Function &F)
  {
    BitVector empty(size, false);

    for (BasicBlock &BB : F)
    {
      struct BBAttributes *attr = new BBAttributes();

      attr->BB = &BB;
      attr->input = empty;
      attr->output = empty;
      attr->genSet = empty;
      attr->killSet = empty;
      attr->tfOut = empty;
      attr->earliest = empty;
      attr->latest = empty;
      attr->postin = empty;

      //Extract Predecessors and Successors
      extractPredSuccBB(&BB, attr);

      //Gets the type of basic block: ENTRY, EXIT, MIDDLE
      if (&BB == &F.getEntryBlock())
      {
        attr->BBType = ENTRY;
      }
      else
      {
        attr->BBType = MIDDLE;
      }

      for (Instruction &II : BB)
      {
        Instruction *I = &II;
        if (dyn_cast<ReturnInst>(I))
        {
          attr->BBType = EXIT;
        }
      }
      BBAttrsMap[&BB] = attr;
    }

    std::map<BasicBlock*, BBAttributes*>::iterator i;

    struct BBAttributes *tmp;
    for (i = BBAttrsMap.begin(); i != BBAttrsMap.end(); i++)
    {
      tmp = BBAttrsMap[i->first];
      if (direction == FORWARD)
      {
          if (tmp->BBType == ENTRY)
          {
              tmp->input = boundary;
          }
          tmp->output = init;
      }
      else if (direction == BACKWARD)
      {
          if (tmp->BBType == EXIT)
          {
              tmp->output = boundary;
          }
          tmp->input = init;
      }
    }
  }
  
  //Stores instructions in seperate vectors for forward and backward propagation
  void Dataflow::flowOrder(Function &F){
    using namespace std;
    stack<BasicBlock*> tempStack;

    for (auto i = po_begin(&F.getEntryBlock()); i != po_end(&F.getEntryBlock()); i++)
    {
        forwardInstructions.push_back(*i);
        tempStack.push(*i);
    }

    while (!tempStack.empty())
    {
        backwardInstructions.push_back(tempStack.top());
        tempStack.pop();
    }
  }

  //Function that performs dataflow analysis iteratively until the output converges
  //Keeps count of the number of iterations
  DFAResult Dataflow::dataflowAnalysis(Function &F){
    
    bool converged = false;
    std::vector<BasicBlock*> block;
    int count = 0;

    //Initialize basic blocks
    BBinit(F);
    //Initialize storage vectors for instructions
    flowOrder(F);
    //Selects appropriate storage vector according to the propagation direction
    if (direction == FORWARD)
    {
      block = backwardInstructions;
    }
    else if (direction == BACKWARD)
    {
      block = forwardInstructions;
    }

    //Iteration starts here
    while (!converged)
    {
      for (BasicBlock *BB : block)
      {
        meetFn(BB);
        // outs() << "meetFn done \n";

        BitVector *input = (direction == FORWARD) ? &BBAttrsMap[BB]->input : &BBAttrsMap[BB]->output;
        // outs() << "input done \n";


        BBAttributes tfResult = transferFn(*input, BB); 
        // outs() << "tf done \n";
      
        BBAttrsMap[BB]->genSet = tfResult.genSet;
        // outs() << "gen done \n";

        BBAttrsMap[BB]->killSet = tfResult.killSet;
        // outs() << "kill done \n";

        BBAttrsMap[BB]->earliest = tfResult.earliest;

        BBAttrsMap[BB]->latest = tfResult.latest;

        BBAttrsMap[BB]->postin = tfResult.postin;

        BitVector *output = (direction == FORWARD) ? &BBAttrsMap[BB]->output : &BBAttrsMap[BB]->input;
        // outs() << "output done \n";

        converged = true;
        if (converged && (tfResult.tfOut != *output))
          converged = false;

        *output = tfResult.tfOut;

      }

      count++;
    }

    //Store final results after convergence
    DFAResult finalResult;
    finalResult.result = BBAttrsMap;
    outs() << "Iterations required for convergence: " << count <<"\n";
    return finalResult;  
  }


  Expression::Expression (Instruction * I) {
    if (BinaryOperator * BO = dyn_cast<BinaryOperator>(I)) {
      this->v1 = BO->getOperand(0);
      this->v2 = BO->getOperand(1);
      this->op = BO->getOpcode();
    }
    else {
      errs() << "We're only considering BinaryOperators\n";
    }
  }
  
  Expression::Expression () {
  }

  // For two expressions to be equal, they must
  // have the same operation and operands.
  bool Expression::operator== (const Expression &e2) const {
    return this->v1 == e2.v1 && this->v2 == e2.v2 && this->op == e2.op;
  }

  // Less than is provided here in case you want
  // to use STL maps, which use less than for
  // equality checking by default
  bool Expression::operator< (const Expression &e2) const {
    if (this->v1 == e2.v1) {
      if (this->v2 == e2.v2) {
        if (this->op == e2.op) {
          return false;
        } else {
          return this->op < e2.op;
        }
      } else {
        return this->v2 < e2.v2;
      }
    } else {
      return this->v1 < e2.v1;
    }
  }
  
  std::string getShortValueName(Value * v) {
    if (v->getName().str().length() > 0) {
      return "%" + v->getName().str();
    }
    else if (isa<Instruction>(v)) {
      std::string s = "";
      raw_string_ostream * strm = new raw_string_ostream(s);
      v->print(*strm);
      std::string inst = strm->str();
      size_t idx1 = inst.find("%");
      size_t idx2 = inst.find(" ",idx1);
      if (idx1 != std::string::npos && idx2 != std::string::npos) {
          return inst.substr(idx1,idx2-idx1);
      }
      else {
        // return "\"" + inst + "\"";
        return "";
      }
    }
//    else if (ConstantInt * cint = dyn_cast<ConstantInt>(v)) {
//      std::string s = "";
//      raw_string_ostream * strm = new raw_string_ostream(s);
//      cint->getValue().print(*strm,true);
//      return strm->str();
//    }
    else {
      std::string s = "";
      raw_string_ostream * strm = new raw_string_ostream(s);
      v->print(*strm);
      std::string inst = strm->str();
      return "\"" + inst + "\"";
    }
  }

  // A pretty printer for Expression objects
  // Feel free to alter in any way you like
  std::string Expression::toString() const {
    std::string op = "?";
    switch (this->op) {
    case Instruction::Add:
    case Instruction::FAdd: op = "+"; break;
    case Instruction::Sub:
    case Instruction::FSub: op = "-"; break;
    case Instruction::Mul:
    case Instruction::FMul: op = "*"; break;
    case Instruction::UDiv:
    case Instruction::FDiv:
    case Instruction::SDiv: op = "/"; break;
    case Instruction::URem:
    case Instruction::FRem:
    case Instruction::SRem: op = "%"; break;
    case Instruction::Shl: op = "<<"; break;
    case Instruction::AShr:
    case Instruction::LShr: op = ">>"; break;
    case Instruction::And: op = "&"; break;
    case Instruction::Or: op = "|"; break;
    case Instruction::Xor: op = "xor"; break;
    }
    return getShortValueName(v1) + " " + op + " " + getShortValueName(v2);
  }

  void copyExpression (Expression *e1, Expression *e2) {
    e1->v1 = e2->v1;
    e1->v2 = e2->v2;
    e1->op = e2->op;
  }

  void printSet(std::vector<Expression> * x) {
    bool first = true;
    outs() << "{";

    for (std::vector<Expression>::iterator it=x->begin(), iend=x->end(); it!=iend; ++it) {
      if (!first) {
        outs() << ", ";
      }
      else {
        first = false;
      }
      outs() << (it->toString());
    }
    outs() << "}\n";
  }

  void printSetBB(std::vector<void *> * x) {
    bool first = true;
    outs() << "{";

    for (std::vector<void *>::iterator it=x->begin(), iend=x->end(); it!=iend; ++it) {
      if (!first) {
        outs() << ", ";
      }
      else {
        first = false;
      }
      if (((BasicBlock*)*it)->hasName())
        outs() << ((BasicBlock *)*it)->getName();
      else
        ((BasicBlock *)*it)->printAsOperand(outs(), false);
    }
    outs() << "}\n";
  }

  void printSetValue(std::vector<void *> * x) {
    bool first = true;
    outs() << "{";

    for (std::vector<void *>::iterator it=x->begin(), iend=x->end(); it!=iend; ++it) {
      if (!first) {
        outs() << ", ";
      }
      else {
        first = false;
      }
      outs() << (getShortValueName((Value *)(*it)));
    }
    outs() << "}\n";
  }

  void printSetInstr(std::vector<void *> * x) {
    bool first = true;
    outs() << "{";

    for (std::vector<void *>::iterator it=x->begin(), iend=x->end(); it!=iend; ++it) {
      if (!first) {
        outs() << ", ";
      }
      else {
        first = false;
      }
      ((Instruction *)(*it))->print(outs());
    }
    outs() << "}\n";
  }

  void printString(std::vector<std::string> *x){
    bool first = true;
    outs() << "{";

    for (std::vector<std::string>::iterator it=x->begin(), iend=x->end(); it!=iend; ++it) {
      if (!first) {
        outs() << ", ";
      }
      else {
        first = false;
      }
      outs() << (*it);
    }
    outs() << "}\n";
  }

  void printBitVector(BitVector bv)
  {
      for(int i = 0 ; i<bv.size(); ++i)
      {
          outs() << (bv[i]);
      }
      outs() << "\n";
  }

  // void printDomain(Domain *d)
  // {

  //   std::vector<Expression> bb_out_set_exp;
  //   outs() << "Domain: ";
  //   for (auto j = 0; j < (*d).size(); ++j)
  //   {
  //     bb_out_set_exp.push_back(*(dd+j));
  //   }
  //   printSet (&bb_out_set_exp);
  //   outs() << "\n";
  // }

}
