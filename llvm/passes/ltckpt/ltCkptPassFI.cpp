/* This pass is not used for the survivability experiments of FIRestarter */

#define DEBUG_TYPE "ltckptSuicide"
#include "ltckpt/ltCkptPassFI.h"

#include <llvm/Support/CommandLine.h>

STATISTIC(NumSuicideSites,         "Number of sites instrumented for suicides");

cl::opt<std::string>
SuicidePointNames("ltckpt-suicide-sites",
       	cl::desc("Specify ':' separated hook names after which suicide-sites to be inserted"),
	cl::value_desc("suicide_points"),
	cl::init(""),
	cl::ValueRequired);

cl::opt<bool>
suicideOnlyOnce("ltckpt-suicide-once",
    	cl::desc("Specify whether to only suicide once per site."),
    	cl::init(true), cl::NotHidden, cl::ValueRequired);

void LtCkptPassFI::getAllLibCallInsts(std::vector<CallInst*> &libCallInsts)
{
   Module::FunctionListType &funcs = this->M->getFunctionList();
   for (Module::iterator mi = funcs.begin(), me = funcs.end(); mi!=me ; ++mi) {
        // for every function that we need to consider, get libcallinsts
	    Function *F = &(*mi);
        if (isInLtckptSection(F)) {
                continue;
        }

        if (F->getName().startswith("__libc"))
                continue;

        if (F->getName().startswith("dl"))
                continue;

        if (F->getName().equals("hypermem_read_impl") ||
                F->getName().equals("hypermem_write_impl"))
                continue;

        for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; I++) {
            Instruction *inst = &(*I);
            CallInst *libCallInst = dyn_cast<CallInst>(inst);
            Function *targetFunc = NULL;
            if (NULL != libCallInst) {
                targetFunc = libCallInst->getCalledFunction();
                if ((NULL != targetFunc)
                   && LTCKPT_IS_EXTERN_FUNC(libCallInst->getCalledFunction())) {
                   libCallInsts.push_back(libCallInst);
                }
            }
        }
    }
}

bool LtCkptPassFI::runOnModule(Module &M)
{
    this->M = &M;

    // Gather all callInsts to TOL-hooks (suicide point functions)

    std::vector<std::string> suicidePointCalleeNames;
    std::stringstream ss(SuicidePointNames);

    std::string item;
    while(std::getline(ss, item, ':')) {
        suicidePointCalleeNames.push_back(item);
    }

    bool suicideAfterLibCalls = false;
    for(unsigned i=0; i < suicidePointCalleeNames.size(); i++) {
        DEBUG(errs() << "Looking for uses of suicide point callee: " << suicidePointCalleeNames[i] << "\n");
        Function *calleeFunc = this->M->getFunction(suicidePointCalleeNames[i]);
        if(NULL == calleeFunc && (!suicideAfterLibCalls)) {
		if (! suicidePointCalleeNames[i].compare("libcalls")) {
			suicideAfterLibCalls = true;
			continue;
		}
	}
#if LLVM_VERSION >= 40
        std::vector<User*> calleeUsers (calleeFunc->user_begin(), calleeFunc->user_end());
#else
        std::vector<User*> calleeUsers (calleeFunc->use_begin(), calleeFunc->use_end());
#endif
        for (unsigned i=0 ; i < calleeUsers.size(); i++) {
            CallInst *CI = dyn_cast<CallInst>(calleeUsers[i]);
            if (NULL != CI) {
                this->suicidePointCallInsts.push_back(CI);
            }
        }
    }
    if (suicideAfterLibCalls) {
        // add libcallinsts to the suicidePointCallInsts.
        std::vector<CallInst*> libCallInsts;
        this->getAllLibCallInsts(libCallInsts);
        suicidePointCallInsts.insert(suicidePointCallInsts.end(), libCallInsts.begin(), libCallInsts.end());
    }
    DEBUG(errs() << "Number of suicide points: " << suicidePointCallInsts.size() << "\n");
    assert(0 != suicidePointCallInsts.size());

    // Get suicide hook
    std::vector<Value*> args(2);
    args[1] = (suicideOnlyOnce) ? CONSTANT_INT(M, 1) : CONSTANT_INT(M, 0);
    Constant *suicideFunc = M.getFunction(LTCKPT_FI_SUICIDE_HOOK_NAME);
    DEBUG(errs() << "Fetched suicideFunc.\n");
    assert(suicideFunc != NULL);
    suicideHook = cast<Function>(suicideFunc);


    // Assign IDs to all the TOL-hooks' sites (suicide point call insts)

    this->lastAssignedSiteId = PassUtil::assignIDs(M, &suicidePointCallInsts, LTCKPT_NAMESPACE_FI);
    assert(0 != lastAssignedSiteId);
    assert(lastAssignedSiteId < LTCKPT_FI_MAX_SUICIDE_SITES);

    // Set initializer for max-suicide-point-size variable
    this->switchboardSizeGV = M.getNamedGlobal(LTCKPT_FI_SUICIDEBOARD_SIZE_VARNAME);
    ConstantInt* const_switchboardsize = CONSTANT_INT(M, lastAssignedSiteId);
    switchboardSizeGV->setInitializer(const_switchboardsize);

    // Initialize the suicide table
    GlobalVariable *suicideBoardGV = M.getGlobalVariable((StringRef) LTCKPT_FI_SUICIDE_BOARD);
    assert(NULL != suicideBoardGV);
    std::vector<Constant*> const_array_elems;
    for (unsigned i=0; i < LTCKPT_FI_MAX_SUICIDE_SITES; i++) {
	const_array_elems.push_back(CONSTANT_INT(M, 1));
    }
    ArrayType* ArrayTy_0 = ArrayType::get(Type::getInt32Ty(M.getContext()), LTCKPT_FI_MAX_SUICIDE_SITES);
    Constant *const_array = ConstantArray::get(ArrayTy_0, const_array_elems);
    suicideBoardGV->setInitializer(const_array);

    CallInst *CI = NULL;
    for (unsigned i=0; i < suicidePointCallInsts.size(); i++) {
	// after every of these callinsts we insert suicide hooks
	Instruction *nextInst = NEXT_INST(dyn_cast<Instruction>(suicidePointCallInsts[i]));
	assert(NULL != nextInst && "Failed to fetch next instruction.");
	uint64_t site_id = PassUtil::getAssignedID(suicidePointCallInsts[i], LTCKPT_NAMESPACE_FI);
	assert(0 != site_id);
	args[0] = CONSTANT_INT(M, site_id);
	CI = PassUtil::createCallInstruction(suicideHook, args, "suicide point", nextInst);
	assert(NULL != CI && "Failed while inserting suicide hook");
	NumSuicideSites++;
    }

    return true;
}


void LtCkptPassFI::getAnalysisUsage(AnalysisUsage &AU) const
{
}

LtCkptPassFI::LtCkptPassFI():LtCkptPass() {
}

char LtCkptPassFI::ID = 0;

RegisterPass<LtCkptPassFI> F("ltckptfi", "Ltckpt Fault Injection Pass");
