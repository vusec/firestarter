#ifndef LTCKPT_LTCKPT_PASS_FI
#define LTCKPT_LTCKPT_PASS_FI 1
#include "ltckpt/ltCkptPass.h"

#define LTCKPT_FI_SUICIDE_BOARD			"ltckpt_suicide_board"
#define LTCKPT_FI_MAX_SUICIDE_SITES		4096
#define LTCKPT_FI_SUICIDEBOARD_SIZE_VARNAME     "ltckpt_suicide_last_site_id"
#define LTCKPT_FI_SUICIDE_HOOK_NAME 		"ltckpt_fi_suicide_per_site"

#define LTCKPT_NAMESPACE_FI			"LTCKPT_FI"

using namespace llvm;
namespace llvm {
		class LtCkptPassFI: public LtCkptPass
		{
			public:
				static char ID;
				LtCkptPassFI();
				virtual bool runOnModule(Module &M);
				virtual void getAnalysisUsage(AnalysisUsage &AU) const;

			protected:
				void getAllLibCallInsts(std::vector<CallInst*> &libCallInsts);

			private:
				Module *M;
				Function *suicideHook;
				GlobalVariable *switchboardSizeGV;
				uint32_t lastAssignedSiteId;
				std::vector<Value*> suicidePointCallInsts;
		};
};


#endif
