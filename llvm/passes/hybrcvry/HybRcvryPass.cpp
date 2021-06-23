/* Description:
 *
 * HybRcvry Pass enables hybrid recovery and supports
 * both ltckpt and tsx based rollbacks further adding 
 * recoverability to both these kinds of recovery windows.
 *
 * The pass expects the following:
 * 1. Functions have been cloned to call either of its two variants
 *    -- ltckpt instrumented or TSX instrumented variant
 *    TOLs are expected right after every library call.
 * 2. Which variant is decided by a run-time global flag.
 *
 * The pass does the following:
 * 1. Enumerate and mark corresponding library calls in
      the two variants of a function
   2. Mark basic-blocks of respective variants, to be able to distinguish
 * 3. In every wrapper function, do:
 *    a. Create a map of library callinsts of corresponding variants.
 *    b. B(s) and B(s)' represent first basic block of respective
 *       recovery windows. They are the basic blocks that have the library call.
 *       Perform rewiring to set their return values to an alloca.
 *    c. Insert per-site gate based conditional branch that lead to:
 *       B(s), B(s)' and a false-block (the error inducing path).
 *    d. Insert call to the setjmp hook before the above compare instruction.
 *    e. At the end of the false-block, insert a conditional branch based on
 *       the global run-time flag to lead to the next basic blocks after the 
 *       TOLs, of respective variants.
 * 4. After every non-libcall call-inst everywhere, insert a conditional branch based on
 *    the global run-time flag to lead to respective variants.
 *
 * Author : Koustubha Bhat
 * Date   : 04-January-2018
 * 
 * Vrije Universiteit, Amsterdam.
 *
 * Revision:
 * HybRcvry, now starts after clone calls have already been inlined
 */

#if LLVM_VERSION >= 37
#define DEBUG_TYPE "hybrcvry"
#endif

#include <hybrcvry/HybRcvryPass.h>

using namespace llvm;

static cl::opt<unsigned long>
rcvryMaxGatesOpt("hybrcvry-max-libcall-gates",
    cl::desc("Specify the maximum number of gates that rcvry static library supports."),
    cl::init(RCVRY_MAX_LIBCALL_SITES), cl::NotHidden, cl::ValueRequired);

static cl::opt<std::string>
clonePrefixOpt("hybrcvry-clone-prefix",
    cl::desc("Specify the clone prefix used (in bbclone pass)."),
    cl::init("bbclone."), cl::NotHidden, cl::ValueRequired);

static cl::list<std::string>
skipSectionsOpt("hybrcvry-skip-sections",
    cl::desc("Specify comma separated names of sections to skip (in bbclone pass)."),
    cl::ZeroOrMore, cl::CommaSeparated, cl::NotHidden, cl::ValueRequired);

static cl::opt<bool>
excludeVariadicFuncsOpt("hybrcvry-exclude-variadic-funcs",
      cl::desc("Opt to exclude variadic functions from being instrumented."),
      cl::init(true), cl::NotHidden, cl::ValueRequired);

static cl::opt<bool>
profilingOpt("hybrcvry-profiling",
    cl::desc("Specify whether to enable recovery-window-gates profiling."),
    cl::init(false), cl::NotHidden, cl::ValueRequired);

static cl::opt<bool>
bbTracingOpt("hybrcvry-profiling-bbtracing",
    cl::desc("Specify whether to enable recovery-window-gates profiling with BB tracing (EDFI)."),
    cl::init(false), cl::NotHidden, cl::ValueRequired);

static cl::opt<bool>
faultInjectionOpt("hybrcvry-inject-fault",
    cl::desc("Specify whether to enable deterministic faults in libcall intervals."),
    cl::init(false), cl::NotHidden, cl::ValueRequired);

static cl::opt<std::string>
endOfWindowHookOpt("hybrcvry-insert-endofwindowhook",
    cl::desc("Specify name of end-of-window-hook to be inserted at each ends of window boundaries (libcall intervals)."),
    cl::init(""), cl::NotHidden, cl::ValueRequired);

cl::list<std::string>
STMLibCallsOpt("hybrcvry-stm-libcalls",
    cl::desc("Specify comma separated list of libcalls that should use STM."),
    cl::ZeroOrMore, cl::CommaSeparated, cl::NotHidden, cl::ValueRequired);

cl::list<std::string>
skipFuncsOpt("hybrcvry-skip-functions",
    cl::desc("Specify comma separated list of functions to skip transformations."),
    cl::ZeroOrMore, cl::CommaSeparated, cl::NotHidden, cl::ValueRequired);

PASS_COMMON_INIT_ONCE();

bool HybRcvryPass::runOnModule(Module &M)
{
    if (0 != HybRcvryPass::PassRunCount)
    {
        errs() << "hybrcvry: Not rerunning this module pass.\n";
        return false;
    }

    this->M = &M;
    DEBUG(errs() << "HybRcvry Pass to enable hybrid recovery paths in library-call intervals.\n");
    this->clonePrefixStr = clonePrefixOpt;
    this->skipSections = &skipSectionsOpt;
    this->rcvryProfiling = profilingOpt;
    this->faultInjection = faultInjectionOpt;
    this->rcvryBBTracing = bbTracingOpt;
    errs() << "endofwindowhook funcname: " << endOfWindowHookOpt << "\n";
    this->endOfWindowHookNameStr = endOfWindowHookOpt;
    this->STMLibCalls = STMLibCallsOpt;
    init();

    GlobalVariable *maxSiteIdGV = this->M->getGlobalVariable((StringRef) HYBRY_GV_NAME_MAX_SITEID);
    assert(NULL != maxSiteIdGV && "maxSiteIdGV not found!");
    DEBUG(errs() << "RCVRY_MAX_LIBCALL_SITES: " << RCVRY_MAX_LIBCALL_SITES << "\n");
    maxSiteIdGV->setInitializer(CONSTANT_INT(*(this->M), RCVRY_MAX_LIBCALL_SITES));
    DEBUG(errs() << "clonePrefixStr : " << clonePrefixStr << "\n");
 
    // bbclone already assigns IDs to libcall insts in every cloned function
    Module::FunctionListType &funcs = M.getFunctionList();
    UnifyFunctionExitNodes UFEN;

    for (Module::iterator it = funcs.begin(); it != funcs.end(); it++) {
        Function *F = &(*it);
        if (F->isIntrinsic() || F->isDeclaration()) {
            continue;
        }
        if (std::string::npos != F->getName().find(clonePrefixStr)) {
            continue;    // skip the cloned functions
        }
        if (isInSkipSection(*F, skipSectionsOpt)) {
            continue;
        }
	    if ((F->isVarArg() && excludeVariadicFuncsOpt)) {
	        continue;
        }
        for (unsigned i=0; i < skipFuncsOpt.size(); i++) {
	        if (0 == F->getName().compare(skipFuncsOpt[i])) {
               continue;
           }
        }
        if ("" != endOfWindowHookOpt) { 
            // endOfWindowHook is to process the delayed frees at the end of the window
            DEBUG(errs() << "Replacing free()s with delayed-frees()\n");
            replaceWithDelayedFrees(*F);
        }
        DEBUG(errs() << "Inserting window entry gates in function: " << F->getName() << "\n");
        insertWindowEntryGates(*F);
        fixupNewAllocaUsages(*F);
	    UFEN.runOnFunction(*F);
    }
    DEBUG(errs() << "Total libcall sites in each variant: " << libCallsMap.size() << "\n");
    return true; 
}

void HybRcvryPass::getAnalysisUsage(AnalysisUsage &AU) const
{
#if LLVM_VERSION >= 37
    AU.addPreserved<DominatorTreeWrapperPass>();
#else
    AU.addPreserved<DominatorTree>();
#endif
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addPreserved<LoopInfoWrapperPass>();
}

char HybRcvryPass::ID = 0;

RegisterPass<HybRcvryPass> H("hybrcvry", "Hybrid Recovery pass");
