/* Description:
 * 
 * This pass makes preparations towards enabling hybrid transactions
 * and recovery mechanisms on target application.
 * It marks library calls, call instructions and local variables of 
 * functions with necessary metadata.
 *
 * Author : Koustubha Bhat
 * Date   : 04-January-2018
 * Vrije Universiteit, Amsterdam.
 */

#if LLVM_VERSION >= 37
#define DEBUG_TYPE "hybprep"
#endif

#include <hybprep/HybPrepPass.h>
#include <llvm/Transforms/Utils/UnifyFunctionExitNodes.h>

using namespace llvm;

static cl::list<std::string>
skipSectionsOpt("hybprep-skip-sections",
    cl::desc("Specify comma separated names of sections to skip."),
    cl::ZeroOrMore, cl::CommaSeparated, cl::NotHidden, cl::ValueRequired);

static cl::opt<std::string>
skipCkptHooksPrefixOpt("hybprep-ckpthooks-prefix",
    cl::desc("Specify prefix of the checkpointing hooks if any, to exclude from instrumentations."),
    cl::init("ltckpt_"), cl::NotHidden, cl::ValueRequired);

static cl::opt<std::string>
removeCkptHooksSectionOpt("hybprep-rm-ckpthooks-from-section",
    cl::desc("Specify section in which checkpointing instrumentation need to be removed."),
    cl::init(""), cl::NotHidden, cl::ValueRequired);

static cl::opt<bool>
markBasicBlocksOpt("hybprep-mark-bbs",
    cl::desc("Specify whether to enumerate and mark basic blocks (for fault injection expts)"),
    cl::init(false), cl::NotHidden, cl::ValueRequired);

PASS_COMMON_INIT_ONCE();

bool HybPrepPass::runOnModule(Module &M)
{
    if (0 != HybPrepPass::PassRunCount)
    {
        errs() << "hybprep: Not rerunning this module pass.\n";
        return false;
    }

    this->M = &M;
    DEBUG(errs() << "Hybprep Pass to make preparations for hybrid checkpointing and recovery\n");

    this->ckptHooksPrefix = skipCkptHooksPrefixOpt;

    Module::FunctionListType &funcs = M.getFunctionList();
    UnifyFunctionExitNodes UFEN;
    std::vector<std::string> removeCkptHooksSections;
    if ("" != removeCkptHooksSectionOpt) {
        removeCkptHooksSections.push_back(removeCkptHooksSectionOpt);
    }

    for (Module::iterator it = funcs.begin(); it != funcs.end(); it++) {
        Function *F = &(*it);
        if (F->isIntrinsic() || F->isDeclaration()) {
            continue;
        }
    
        if (PassUtil::isInAnyOfSections(*F, removeCkptHooksSections)) {
            removeCkptHooks(*F);
        }

        if (PassUtil::isInAnyOfSections(*F, skipSectionsOpt)) {
            continue;
        }

	UFEN.runOnFunction(*F);
        DEBUG(errs() << "Marking library calls in function: " << F->getName() << "\n");
        markLibCalls(*F);
        DEBUG(errs() << "Marking call insts in function: " << F->getName() << "\n");
        markCallInsts(*F);
        DEBUG(errs() << "Marking local variables in function: " << F->getName() << "\n");
        markLocals(*F);
        if (markBasicBlocksOpt) {
            markBasicBlocks(*F);
        }
        markInstructions(*F);
    }
    // Assign overall IDs to all the libcalls
    uint64_t totalLCs = PassUtil::assignIDs(*(this->M), &(allLibCallInsts), HYBPREP_NAMESPACE_ALL_LIBCALLS);
    DEBUG(errs() << "Total library call insts: " << totalLCs << "\n");
    return true; 
}

void HybPrepPass::getAnalysisUsage(AnalysisUsage &AU) const
{
#if LLVM_VERSION >= 37
    AU.addPreserved<DominatorTreeWrapperPass>();
#else
    AU.addPreserved<DominatorTree>();
#endif
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addPreserved<LoopInfoWrapperPass>();
}

char HybPrepPass::ID = 0;

RegisterPass<HybPrepPass> HP("hybprep", "Preparation pass for Hybrid Tx and Recovery");
