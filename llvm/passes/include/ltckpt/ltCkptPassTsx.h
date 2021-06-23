#ifndef LTCKPT_LTCKPT_PASS_TSX
#define LTCKPT_LTCKPT_PASS_TSX 1
#include <common/tx_common.h>
#include "ltckpt/ltCkptPass.h"

#define TSX_START_FUNC_NAME "ltckpt_top_of_the_loop_tsx"
#define TSX_END_FUNC_NAME   "ltckpt_end_of_window_tsx"

using namespace llvm;
namespace llvm {
    class LtCkptPassTsx: public LtCkptPass
    {
        public:
            static char ID;
            LtCkptPassTsx();
            virtual bool runOnModule(Module &M);
            virtual void getAnalysisUsage(AnalysisUsage &AU) const;

        protected:
            Function *tsxStartFunc, *tsxEndFunc;
            std::set<Instruction*> txWindowStarts;
            std::set<Instruction*> txWindowEnds;
            bool skipInstrumentation(Function &F);
            void markTxWindowSizeBasedBoundaries(Function &F);
            void markLibCallIntervalBasedBoundaries(Function &F);
            void insertTSXBoundaryHooks();
    };
};


#endif
