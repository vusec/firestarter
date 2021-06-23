#ifndef LTCKPT_LTCKPT_PASS_BASIC
#define LTCKPT_LTCKPT_PASS_BASIC 1
#include "ltckpt/ltCkptPass.h"

#if LLVM_HAS_DSA
#include <dsa/DataStructure.h>
#include <dsa/DSGraph.h>
#endif

using namespace llvm;
namespace llvm {
		class LtCkptPassBasic: public LtCkptPass
		{
			public:
				static char ID;
				LtCkptPassBasic();
				virtual bool runOnModule(Module &M);
				virtual void getAnalysisUsage(AnalysisUsage &AU) const;

			protected:
#if LLVM_HAS_DSA
				DataStructures *EQDS;
#endif
				bool doubleCheckStore(StoreInst *inst);
				bool virtual storeToBeInstrumented(Instruction *inst);
				virtual bool onFunction(Function &F);
				virtual bool memIntrinsicToBeInstrumented(Instruction *inst);
				void eliminateDoubleStores(std::set<Instruction *> *il, Function *F);
				void aggregateStores(std::set<Instruction *> *il,
				                     std::set<Value *> *oMap,
														 Function *F);
		};
};


#endif
