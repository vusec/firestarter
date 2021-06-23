#ifndef HYBPREP_PASS_H
#define HYBPREP_PASS_H

#define HYBPREP_NAMESPACE_ALL_LIBCALLS	"HYBPREP_ALL_LIBCALLS"
#define HYBPREP_NAMESPACE_LIBCALLS		"HYBPREP_LIBCALL"
#define HYBPREP_NAMESPACE_CALLINSTS		"HYBPREP_CI"
#define HYBPREP_NAMESPACE_LOCALS		"HYBPREP_LOCALS"
#define HYBPREP_NAMESPACE_BBS		    "HYBPREP_BBS"
#define HYBPREP_NAMESPACE_FUNC_INSTRS   "HYBPREP_I"

#if LLVM_VERSION >= 37
#ifndef DEBUG_TYPE
#define DEBUG_TYPE "hybprep"
#endif
#endif
#include <pass.h>

using namespace llvm;

namespace llvm
{

STATISTIC(NumLibCallsMarked,          "Number of libcalls marked.");
STATISTIC(NumCallInstsMarked,         "Number of call insts marked.");
STATISTIC(NumLocalsMarked,            "Number of local variables marked.");
STATISTIC(NumBBsMarked,               "Number of basic blocks marked.");
STATISTIC(NumInstsMarked,             "Number of instructions marked.");
STATISTIC(NumLtckptHooksRemoved,      "Number of checkpointing callinsts removed (from specified section).");

class HybPrepPass : public ModulePass
{
public:
    static char ID;
    static unsigned PassRunCount;

    HybPrepPass();
    virtual bool runOnModule(Module &M);
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

protected:
    unsigned markLibCalls(Function &F);
    uint64_t markCallInsts(Function &F);
    uint64_t markLocals(Function &F);
    uint64_t markBasicBlocks(Function &F);
    uint64_t markInstructions(Function &F);
    void removeCkptHooks(Function &F);

private:
    Module *M;
    std::vector<std::string> *skipSections;
    std::vector<Value*> allLibCallInsts;
    std::string ckptHooksPrefix;

    void init();
    bool isInSkipSection(Function &F, std::vector<std::string> &sectionsList);
};

unsigned HybPrepPass::PassRunCount = 0;

HybPrepPass::HybPrepPass() : ModulePass(ID) {}

unsigned HybPrepPass::markLibCalls(Function &F)
{
    std::vector<Value *> libCallInsts;
    for (inst_iterator I = inst_begin(&F), E = inst_end(&F); I != E; I++) {
        CallInst *CI = dyn_cast<CallInst>(&(*I));
        if (NULL != CI) {
            if (PassUtil::isLibCallInst(CI)) {
                libCallInsts.push_back(CI);
            }
        }
    }
    NumLibCallsMarked += libCallInsts.size();
    PassUtil::assignIDs(*M, &libCallInsts, HYBPREP_NAMESPACE_LIBCALLS); // reset counter
    DEBUG(errs() << "Number of libcalls found in function: " << F.getName() << " = "
                 << libCallInsts.size() << "\n");
    // Add these to the overall libcalls list
    allLibCallInsts.insert(allLibCallInsts.end(), libCallInsts.begin(), libCallInsts.end());
    return libCallInsts.size();
}

uint64_t HybPrepPass::markCallInsts(Function &F)
{
    // Mark all the call instructions except libcalls, in the cloned functions
    std::vector<Value *> callInsts;
    for (inst_iterator I = inst_begin(&F);
         I != inst_end(&F); I++) {
         CallInst *CI = dyn_cast<CallInst>(&(*I));
         if (NULL != CI && (false == PassUtil::isLibCallInst(CI))) {
             Function *calledFunc = CI->getCalledFunction();
             if (NULL != calledFunc && (false == calledFunc->isIntrinsic())) {
                 if (("" == this->ckptHooksPrefix)
                    || (std::string::npos == calledFunc->getName().find(this->ckptHooksPrefix))) {
                    DEBUG(errs() << "Adding callinst to: " << calledFunc->getName() << "\n");
                    callInsts.push_back(CI);
                 }
             }
         }
    }
    NumCallInstsMarked += callInsts.size();
    DEBUG(errs() << "Number of call insts found in function" << F.getName()
                 << " = " << callInsts.size() << "\n");
    return PassUtil::assignIDs(*M, &callInsts, HYBPREP_NAMESPACE_CALLINSTS,
                        "_LAST_ID_HOLDER", 0, true); // reset counter
}

uint64_t HybPrepPass::markLocals(Function &F)
{
    std::vector<Value *> localVarsDefs;

#if LLVM_VERSION >= 37
    DominatorTree *DT = &getAnalysis<DominatorTreeWrapperPass>(F).getDomTree();
#else
    DominatorTree *DT = &getAnalysis<DominatorTree>(F);
#endif
    for(inst_iterator it = inst_begin(&F), E = inst_end(&F); it != E; it++) {
        Instruction *I = &(*it);
        std::vector<User *> users;
#if LLVM_VERSION >= 40
        users.assign(I->user_begin(), I->user_end());
#else
        users.assign(I->use_begin(), I->use_end());
#endif
        if (users.empty()) continue;
        bool isDef = true;
        for (unsigned i=0; i < users.size(); i++) {
            for (User::op_iterator IU = users[i]->op_begin(), EU = users[i]->op_end();
                 IU != EU; IU++) {
                if (false == DT->dominates(I, *IU)) {
                    isDef = false;
                }
            }
        }
        if (isDef) {
            localVarsDefs.push_back(I);
        }
    }
    NumLocalsMarked += localVarsDefs.size();
    DEBUG(errs() << "Number of local variables in function: " << F.getName() << " = "
                 << localVarsDefs.size() << "\n"); 
    return PassUtil::assignIDs(*M, &localVarsDefs, HYBPREP_NAMESPACE_LOCALS, "_LAST_ID_HOLDER", 0, true);
}

uint64_t HybPrepPass::markBasicBlocks(Function &F)
{
    // Mark all the basic blocks in the function
    std::vector<Value *> BBs;
    for (Function::iterator IB = F.begin(), EB = F.end(); IB != EB; IB++){
         BBs.push_back(dyn_cast<Value>(&(*IB)));
    }
    NumBBsMarked += BBs.size();
    DEBUG(errs() << "Number of call insts found in function" << F.getName()
                 << " = " << BBs.size() << "\n");
    return PassUtil::assignIDs(*M, &BBs, HYBPREP_NAMESPACE_BBS,
                        "_LAST_ID_HOLDER", 0, false); // dont reset counter
}

uint64_t HybPrepPass::markInstructions(Function &F)
{
    std::vector<Value *> Insts;
    for (inst_iterator I = inst_begin(&F), E = inst_end(&F); I != E; I++) {
        Insts.push_back(&(*I));
    }
    NumInstsMarked += Insts.size();
    DEBUG(errs() << "Number of instructions in function: " << F.getName()
                 << " = " << Insts.size() << "\n");
    std::string lastIDHolder = std::string("_LAST_ID_").append(F.getName());
    return PassUtil::assignIDs(*M, &Insts, HYBPREP_NAMESPACE_FUNC_INSTRS,
                               lastIDHolder, 0, true); // Reset the counter per function
}

void HybPrepPass::removeCkptHooks(Function &F)
{
    std::vector<CallInst*> hooksToRemove;
    for (inst_iterator I = inst_begin(&F), E = inst_end(&F); I != E; I++) {
        CallInst *CI = dyn_cast<CallInst>(&(*I));
        if (NULL != CI) {
            Function *calledFunc = CI->getCalledFunction();
            if (std::string::npos != calledFunc->getName().find(this->ckptHooksPrefix)) {
                hooksToRemove.push_back(CI);
            }
        }
    }
    NumLtckptHooksRemoved += hooksToRemove.size();

    for (unsigned i=0; i < hooksToRemove.size(); i++) {
        hooksToRemove[i]->eraseFromParent();
    }
    return;
}
}
#endif
