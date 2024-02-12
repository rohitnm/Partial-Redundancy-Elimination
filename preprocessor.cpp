#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
using namespace llvm;

namespace {
  class preprocessor : public FunctionPass {
  public:
    static char ID;
    preprocessor() : FunctionPass(ID) {}
	~preprocessor() {}    
    bool runOnFunction(Function &F) override
    {
		std::vector<BasicBlock*> originalBlockList;
		std::vector<BasicBlock*> newBlockList;
		std::vector<BasicBlock*> newAddBlockList;

		for (auto& block : F) {
			originalBlockList.push_back(&block);
		}
		outs() << "BB og size : " << originalBlockList.size() << "\n";

		for (auto &tmp : originalBlockList) {
			BasicBlock &block = *tmp;
			if (block.size() > 1) {
				bool first = true;
				for (int i = block.size() - 1; i > 0; --i) {
					if (first) {
						SplitBlock(&block, &block.back());
						first = false;
					} else {
						SplitBlock(&block, &(*(++block.rbegin())));
					}
				
				}
			}
		}
		for (auto& block : F) {
			newBlockList.push_back(&block);
		}
    	outs() << "BB new size : " << newBlockList.size() << "\n";

		std::set<std::pair<BasicBlock*, BasicBlock*>> splitBBPairs;

		for (auto& block : F) {
			if (block.hasNPredecessorsOrMore(2)) {
				for (auto pred : predecessors(&block)) {
					splitBBPairs.insert(std::make_pair(pred, &block));
				}
			}
		}
		for (auto& pair : splitBBPairs) {
			SplitEdge(pair.first, pair.second);
		}

		// for (auto& block : F) {
		// 	newAddBlockList.push_back(&block);
		// }
    	// outs() << "BB new size : " << newAddBlockList.size() << "\n";
  }
};
}  
char preprocessor::ID = 0;
static RegisterPass<preprocessor> X("preprocessor", "Split basic blocks & add new blocks at critical edges");


