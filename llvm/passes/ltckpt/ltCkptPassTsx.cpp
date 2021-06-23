/* Description: LLVM instrumentation pass to insert calls to Intel TSX funcs
 *              where HW transactions (HTM) are possible and beneficial
 * Pass does the following:
 * 1. Identify all instructions with positive-int TXWINDOWSIZE metadata 
 *      - these are CallInsts with statically known worst-case Tx size
 * 2. Surround with tx-end and tx-start
 *      - insert call to tx-start() after TXWINDOWSIZE instruction
 *      - insert call to tx-end() before TXWINDOWEND instruction
 *
 * Author : Dmitrii Kuvaiskii
 * Date	  : 12-April-2017
 * TU Dresden, Germany.
 */
#define DEBUG_TYPE "ltckpttsx"

#include "ltckpt/ltCkptPassTsx.h"
#include <llvm/Support/CommandLine.h>

#define DEFAULT_THRESHOLD 1000

#define TXWINDOW_SIZE_NAMESPACE_NAME "TXWINDOWSIZE"
#define TXWINDOW_END_NAMESPACE_NAME "TXWINDOWEND"

#define NEXT_INST(I)        ({              \
    BasicBlock::iterator BI(I);             \
    if (++BI == I->getParent()->end()) {    \
        NULL;                               \
    }                                       \
    &*BI;                                   \
    })

STATISTIC(NumTxWindowSizes, "Number of TXWINDOWSIZE instructions identified");
STATISTIC(NumTxWindowEnds,  "Number of TXWINDOWEND instructions identified");

cl::opt<int> ltckptTsxThreshold("ltckpt_tsx_threshold",
                                   cl::desc("Threshold on maximum TSX transaction size (int)"),
                                   cl::value_desc("TSX threshold"),
                                   cl::init(DEFAULT_THRESHOLD));

cl::opt<bool> ltckptTsxWindowProfiling("ltckpt_tsx_window_profiling",
    cl::desc("Specify to enable TSX window profiling. (Ensure ltckpt static lib compatibility)"),
    cl::init(false), cl::value_desc("window profiling"));

cl::opt<bool> ltckptAllTOLs("ltckpt_tsx_no_threshold",
			    cl::desc("Set no threshold on TSX transaction size (overrides ltckpt_tsx_threshold option)"),
			    cl::value_desc("No TSX threshold"), cl::init(false));

cl::opt<std::string> ltckptTSXInstrumentSectionName("ltckpt_tsx_instrument_section",
                                    cl::desc("Specify name of a particular section to instrument"),
                                    cl::value_desc("section name"),
                                    cl::init(""), cl::ValueRequired);

static cl::opt<bool>
excludeVariadicFuncsOpt("ltckpt_tsx-exclude-variadic-funcs",
      cl::desc("Opt to exclude variadic functions from being instrumented."),
      cl::init(false), cl::NotHidden, cl::ValueRequired);

static cl::list<std::string>
excludeFuncsOpt("ltckpt_tsx-exclude-funcs",
    cl::desc("Specify all the comma-separated function names to exclude from being instrumented."),
    cl::ZeroOrMore, cl::CommaSeparated, cl::NotHidden, cl::ValueRequired);

bool LtCkptPassTsx::runOnModule(Module &M)
{
    this->M = &M;
    this->tolSuffix = "tsx";
    this->txWindowStarts.clear();
    this->txWindowEnds.clear();

    std::set<Function*> excludeFuncs;
    for (unsigned i=0;i<excludeFuncsOpt.size();i++) {
        Function *exF = this->M->getFunction(excludeFuncsOpt[i]);
        assert(NULL != exF);
        if (0 == excludeFuncs.count(exF)) {
            excludeFuncs.insert(exF);
        }
    }
    errs()<< "Exclude funcs size is: " << excludeFuncs.size() << "\n";

    for (Module::iterator mi = M.begin(), me = M.end(); mi != me; ++mi) {
        Function *F = &*mi;
        if (!F->isDeclaration() && !(F->empty())) {
            if ((F->isVarArg() && excludeVariadicFuncsOpt)
                    || (0 != excludeFuncs.count(F))) {
                    txWindowEnds.insert(F->getEntryBlock().getFirstNonPHI());
                    errs() << "marked function: " << F->getName() << " for txWindowEnd\n";
                    continue;
            }
        }
        if (skipInstrumentation(*F)) {
            continue;
        }
        if (ltckptAllTOLs) {
            /* Add TSX boundaries to all libcall intervals */
            markLibCallIntervalBasedBoundaries(*F);
        } else {
            /* Add TSX boundaries based on static analysis results
            * from TxWindowPass - manifested as metadata on certian
            * selection instructions.
            */
            markTxWindowSizeBasedBoundaries(*F);
        }
    }

    insertTSXBoundaryHooks();

    if (ltckptTsxWindowProfiling) {
                this->lastAssignedProfilingId = initializeWindowProfiling(tsxStartFunc,
                                                        LTCKPT_NAMESPACE_TOL "_" + this->tolSuffix, this->lastAssignedProfilingId + 1, 0);
                errs() << "Window Profiling: Last assigned ID for " << LTCKPT_NAMESPACE_TOL "_" + this->tolSuffix << " : " << this->lastAssignedProfilingId << "\n";
                this->lastAssignedProfilingId = initializeWindowProfiling(tsxEndFunc,
                                                        LTCKPT_NAMESPACE_TOL "_endtsx", this->lastAssignedProfilingId + 1, 0);
                errs() << "Window Profiling: Last assigned ID for " << LTCKPT_NAMESPACE_TOL "_endtsx" << " : " << this->lastAssignedProfilingId << "\n";
    }
    return true;
}

bool LtCkptPassTsx::skipInstrumentation(Function &F)
{
     if (isInLtckptSection(&F)) {
        return true;
     }

     if ("" != ltckptTSXInstrumentSectionName
         && (!isInSection(&F, ltckptTSXInstrumentSectionName))) {
         return true;
     }

     if (F.getName().startswith("__libc"))
        return true;

    if (F.getName().startswith("dl"))
        return true;

    if (F.getName().equals("hypermem_read_impl") ||
        F.getName().equals("hypermem_write_impl"))
        return true;

    return false;
}

void LtCkptPassTsx::insertTSXBoundaryHooks()
{
    this->tsxStartFunc = this->M->getFunction(TSX_START_FUNC_NAME);
    this->tsxEndFunc = this->M->getFunction(TSX_END_FUNC_NAME);
    assert(tsxStartFunc != NULL);
    assert(tsxEndFunc != NULL);

    std::vector<Value*> args(0), profArgs(1);
    for (auto it = txWindowStarts.begin(); it != txWindowStarts.end(); it++) {
        // start transaction after this I (i.e., before next instruction)
        Instruction *I = *it;
        Instruction *CI = NULL;
	DEBUG(errs() << "txWindowStart in function: " << I->getParent()->getParent()->getName() << "\n");
        if (ltckptTsxWindowProfiling) {
                profArgs[0] = ZERO_CONSTANT_INT(*(this->M));
                CI = PassUtil::createCallInstruction(tsxStartFunc, profArgs, "", NEXT_INST(I));
        } else {
                CI = PassUtil::createCallInstruction(tsxStartFunc, args, "", NEXT_INST(I));
        }
        CI->setMetadata(LTCKPT_NAMESPACE_TX_TYPE, PassUtil::createMDNodeForConstant(this->M, TX_TYPE_TSX));
    }

    for (auto it = txWindowEnds.begin(); it != txWindowEnds.end(); it++) {
        // end transaction before this I
        Instruction *I = *it;
        if (ltckptTsxWindowProfiling) {
                profArgs[0] = ZERO_CONSTANT_INT(*(this->M));
                PassUtil::createCallInstruction(tsxEndFunc, profArgs, "", I);
        } else {
                PassUtil::createCallInstruction(tsxEndFunc, args, "", I);
        }
    }

    NumTxWindowSizes = txWindowStarts.size();
    NumTxWindowEnds  = txWindowEnds.size();
}

/*
 * Use metadata set by TxWindowPass to mark transaction boundaries
 * based on instruction count within them
 */
void LtCkptPassTsx::markTxWindowSizeBasedBoundaries(Function &F)
{
    // iterate through all instructions and memorize tx-window-start and tx-window end places
    for (Function::iterator fi = F.begin(); fi != F.end(); ++fi) {
        BasicBlock *BB = &*fi;
        for (BasicBlock::iterator bi = BB->begin(); bi != BB->end(); ++bi) {
            Instruction *I = &*bi;

            MDNode *N = I->getMetadata(TXWINDOW_SIZE_NAMESPACE_NAME);
            if (N && N->getNumOperands() > 0) {
                ConstantInt *CI = dyn_cast_or_null<ConstantInt>(((ConstantAsMetadata *)((Metadata *)(N->getOperand(0))))->getValue()) ;
                int64_t size = CI->getSExtValue();
                if (size > 0 && size < ltckptTsxThreshold) {
                    // found a transaction with size under threshold -> will start TSX here
                    txWindowStarts.insert(I);
                }
            }
            N = I->getMetadata(TXWINDOW_END_NAMESPACE_NAME);
            if (N && N->getNumOperands() > 0) {
                ConstantInt *CI = dyn_cast_or_null<ConstantInt>(((ConstantAsMetadata *)((Metadata *)(N->getOperand(0))))->getValue()) ;
                int64_t flag = CI->getSExtValue();
                if (flag) {
                    // found a place to end the (possible) TSX transaction -> will end TSX here
                    txWindowEnds.insert(I);
                }
            }
        } // forach inst, ends
    } // foreach BB, ends
    return;
}

/* 
 * Mark TSX boundaries for every libcall interval
 * found in the function
 * Note: Do not use both of these marking strategies. 
 *       They must be mutually exclusive.
 * 
 * Feasibility study for this strategy, done by Mihai-Andrei(Mike) Spadaru.
 */
void LtCkptPassTsx::markLibCallIntervalBasedBoundaries(Function &F)
{
    for (Function::iterator fi = F.begin(); fi != F.end(); ++fi) {
        BasicBlock *BB = &*fi;
        for (BasicBlock::iterator bi = BB->begin(); bi != BB->end(); ++bi) {
            Instruction *I = &*bi;
            CallInst *libCallInst = dyn_cast<CallInst>(I);
            Function *targetFunc = NULL;
            if (NULL != libCallInst) {
                targetFunc = libCallInst->getCalledFunction();
                if ((NULL != targetFunc)
                    && LTCKPT_IS_EXTERN_FUNC(libCallInst->getCalledFunction())) {
                    /* Skip if this libcall is not used anywhere */
                    std::vector<User *> users;
                    if (PassUtil::callHasNoUsers(libCallInst, users)) {
                        DEBUG(errs() << "\t Skipping instrumentation for libcall: " << targetFunc->getName() << "\n");
                        continue;
                    }
                    if (TxUtil::isNonFaultableLibCall(libCallInst)) {
                        DEBUG(errs() << "\t Skipping non-faultable libcall: " << targetFunc->getName() << "\n");
                        continue;
                    }
                    /* Mark TSX begin */
                    txWindowStarts.insert(I);

                    /* Mark TSX end */
                    txWindowEnds.insert(I);
                }
           }
        #if 0   // Why should we end a transaction at the return of a function? Doesnt make sense now
                // since, we want to continue the transaction until the next transaction that begins at
                // the place where next library call happens. Otherwise, region between after the return
                // at the callsite and the next library call remains unprotected.
           else {
               ReturnInst *retInst = dyn_cast<ReturnInst>(I);
               if (NULL != retInst) {
                   txWindowEnds.insert(I);
               }
           }
        #endif
         } // for each instr, ends.
    } // for each BB, ends
    return;
}

void LtCkptPassTsx::getAnalysisUsage(AnalysisUsage &AU) const
{
}

LtCkptPassTsx::LtCkptPassTsx():LtCkptPass() {
}

char LtCkptPassTsx::ID = 0;

RegisterPass<LtCkptPassTsx> TSX("ltckpttsx", "Intel TSX Checkpointing Pass");
