#ifndef LTCKPT_LTCKPT_PASS_HEAP_H
#define LTCKPT_LTCKPT_PASS_HEAP_H
#include "ltckpt/ltCkptPassBasic.h"
#include <dsa/DataStructure.h>
#include <dsa/DSGraph.h>
#include <llvm/Analysis/Dominators.h>

using namespace llvm;


namespace llvm {
		class LtCkptPassHeap: public LtCkptPassBasic
		{
			public:
				static char ID;
				LtCkptPassHeap();
				virtual bool runOnModule(Module &M);
				virtual void getAnalysisUsage(AnalysisUsage &AU) const;
			private:
				EQTDDataStructures *EQDS;
				DominatorTree *DT;
				bool doubleCheckStore(StoreInst *inst);
				bool predsearch_store(BasicBlock *BB, AliasAnalysis::Location &loc,
                            std::set<BasicBlock *> *visited, StoreInst *inst);
				bool virtual storeToBeInstrumented(Instruction *inst);
				bool virtual memIntrinsicToBeInstrumented(Instruction *inst);
		};
};


#endif
