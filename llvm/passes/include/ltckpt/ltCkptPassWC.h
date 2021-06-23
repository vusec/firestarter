#ifndef LTCKPT_LTCKPT_PASS_WC
#define LTCKPT_LTCKPT_PASS_WC
#include "ltckpt/ltCkptPassBasic.h"

using namespace llvm;

namespace llvm {
		class LtCkptPassWC: public LtCkptPassBasic
		{
			public:
				static char ID;
				LtCkptPassWC();
			private:
				bool virtual storeToBeInstrumented(Instruction *inst);
				bool virtual memIntrinsicToBeInstrumented(Instruction *inst);
		};
};


#endif
