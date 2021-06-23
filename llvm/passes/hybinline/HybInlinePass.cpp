/* Description:
 * 
 * This pass makes preparations towards enabling hybrid transactions
 * and recovery mechanisms on target application.
 * After HybPrepPass marks library calls, call instructions and local variables of 
 * functions with necessary metadata, this pass inlines the call instructions to 
 * the clones in its container functions.
 *
 * Author : Koustubha Bhat
 * Date   : 12-March-2018
 * Vrije Universiteit, Amsterdam.
 */

#if LLVM_VERSION >= 37
#define DEBUG_TYPE "hybinline"
#endif

#include <hybinline/HybInlinePass.h>

using namespace llvm;

static cl::opt<std::string>
clonePrefixOpt("hybry-clone-prefix",
    cl::desc("Specify the clone prefix used (in bbclone pass)."),
    cl::init("bbclone."), cl::NotHidden, cl::ValueRequired);

static cl::list<std::string>
skipSectionsOpt("hybinline-skip-sections",
    cl::desc("Specify comma separated names of sections to skip."),
    cl::ZeroOrMore, cl::CommaSeparated, cl::NotHidden, cl::ValueRequired);

PASS_COMMON_INIT_ONCE();

bool HybInlinePass::runOnModule(Module &M)
{
    if (0 != HybInlinePass::PassRunCount)
    {
        errs() << "hybry: Not rerunning this module pass.\n";
        return false;
    }

    this->M = &M;
    DEBUG(errs() << "HybInline Pass to inline cloned function variants.\n");
    this->clonePrefixStr = clonePrefixOpt;
    this->skipSections = &skipSectionsOpt;

    // bbclone already assigns IDs to libcall insts in every cloned function
    Module::FunctionListType &funcs = M.getFunctionList();
    for (Module::iterator it = funcs.begin(); it != funcs.end(); it++) {
        Function *F = &(*it);
        if (F->isIntrinsic() || F->isDeclaration()) {
            continue;
        }
        if (std::string::npos != F->getName().find(clonePrefixStr)) {
            continue;    // skip the cloned functions
        }
        if (PassUtil::isInAnyOfSections(*F, skipSectionsOpt)) {
            continue;
        }
        DEBUG(errs() << "Inlining calls to cloned variants in function: " << F->getName() << "\n");
        inlineCloneCalls(*F);
    }
    return true;
}

void HybInlinePass::getAnalysisUsage(AnalysisUsage &AU) const
{
}

char HybInlinePass::ID = 0;

RegisterPass<HybInlinePass> HI("hybinline", "Inliner pass for Hybrid Tx and Recovery");
