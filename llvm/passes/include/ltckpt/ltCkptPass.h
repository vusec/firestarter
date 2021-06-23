#ifndef LTCKPT_LTCKPT_H
#define LTCKPT_LTCKPT_H

#include <pass.h>
#include <list>
#include <assert.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <common/tx_common.h>

#define LTCKPT_GET_HOOK(M, LM, LH) M.getFunction(LH "_" + LM);

#define CONF_SETUP_FUNC_NAME "ltckpt_conf_setup"
#define CONF_SETUP_FUNC_NAME_VM "ltckpt_conf_setup_vm"

#define STORE_HOOK_NAME "ltckpt_store_hook"
#define TOP_OF_THE_LOOP_FUNC_NAME "ltckpt_top_of_the_loop"
#define END_OF_THE_WINDOW_FUNC_NAME "ltckpt_end_of_window"

#define MEMCPY_FUNC_NAME  "ltckpt_memcpy_hook"

#define LTCKPT_NAMESPACE_TX_TYPE	"LTCKPT_TX_TYPE"
#define LTCKPT_NAMESPACE_TOL		"LTCKPT_PROFILING_TOL"
#define LTCKPT_NAMESPACE_STOREHOOK	"LTCKPT_PROFILING_STOREHOOK"
#define LTCKPT_NAMESPACE_MEMCPYHOOK	"LTCKPT_PROFILING_MEMCPYHOOK"

#define LTCKPT_ASM_SAVE_REGS_FUNC_NAME          "ltckpt_asm_save_registers"
#define LTCKPT_ASM_SAVE_REGS_DUMMY_FUNC         "ltckpt_asm_save_registers_type"
#define LTCKPT_SETJMP_SAVE_REGS_DUMMY_FUNC      "ltckpt_setjmp_save_registers_type"
#define LTCKPT_ASM_SAVE_REGS_BUFFER             "ltckpt_registers"
#define LTCKPT_FP_ADDR1_GV			"ltckpt_fp_addr1"
#define LTCKPT_FP_ADDR2_GV			"ltckpt_fp_addr2"

#define ltckptPassLog(M) DEBUG(dbgs()  << M )
#define WORST_CASE_SCENARIO 0

#define LTCKPT_IS_EXTERN_FUNC(F)        (                                       \
                (!F->isIntrinsic())                                             \
                && (F->isDeclaration() || (NULL == &((F)->getEntryBlock())))    \
        )

using namespace llvm;

namespace llvm {

	class LtCkptPass : public ModulePass {
		public:
			static char ID;
			LtCkptPass();
			LtCkptPass(char ID);
			virtual bool runOnModule(Module &M);
			virtual void getAnalysisUsage(AnalysisUsage &AU) const;
			void instrumentRange(Value *ptr, APInt from, APInt size,  Instruction *inst);

		protected:
			Function *confSetupInitHook;
			Function *confSetupHook;
			Function *vmLateInitHook;
			Function *storeInstHook;
			Function *memcpyHook;
			Function *topOfTheLoopHook;
			Function *endOfWindowHook;
			Function *tsxStartHook;
			Function *tsxEndHook;
			Function *saveRegsHook;
			std::vector<GlobalVariable*> globalVariables;
			std::string tolSuffix;
			bool isInLtckptSection(Function *f);
			bool isInSection(Function *F, std::string sectionName);
			void instrumentRetAddr(Instruction *insertPt);
			void instrumentStore(Instruction *inst);
			void instrumentMemIntrinsic(Instruction *inst);
			bool instrumentTopOfTheLoop(Function &F);
			bool instrumentLibCallCkpts(Function &F);
			bool instrumentConfSetup(Function &F);
			bool instructionModifiesVar(Module &M, Instruction *inst, GlobalVariable* var);
			void fetchLastTwoFPs(Instruction *insertPt);
			virtual bool onFunction(Function &F);
			bool virtual storeToBeInstrumented(Instruction *inst);
			bool virtual memIntrinsicToBeInstrumented(Instruction *inst);
			uint64_t initializeWindowProfiling(Function *hook, std::string namespaceName, uint64_t startId, unsigned siteIdArgNum);
			void fetchFPAndSP(Instruction *insertPt);
			AliasAnalysis *AA;
#if LLVM_VERSION >= 37
			const DATA_LAYOUT_TY *DL;
#else
			DATA_LAYOUT_TY *DL;
#endif
		    	Module *M;
			uint64_t lastAssignedProfilingId = 0;

		private:
			void createHooks(Module &M);
			void createSaveRegsHook(std::vector<Value *> &svRegsArgs, Instruction *I);
                        bool isExternalFunc(Function *F);
			void initializeWindowProfiling(Function *hook, std::string namespaceName, unsigned siteIdArgNum);
	};

} /* namespace llvm */

#endif
