#ifndef HYBINLINE_PASS_H
#define HYBINLINE_PASS_H

#if LLVM_VERSION >= 37
#ifndef DEBUG_TYPE
#define DEBUG_TYPE "hybinline"
#endif
#endif
#include <pass.h>
#include <hybprep/HybPrepPass.h>
#include <hybrcvry/HybRcvryCommon.h>

using namespace llvm;

namespace llvm
{

STATISTIC(NumCloneCallsInlined,                  "Number of calls to cloned function variants inlined.");

class HybInlinePass : public ModulePass
{
public:
    static char ID;
    static unsigned PassRunCount;

    HybInlinePass();
    virtual bool runOnModule(Module &M);
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

protected:
    std::map<CallInst *, CallInst *> libCallsMap;
    std::map<CallInst *, CallInst *> CIsMap;
    std::map<Value *, Value *> localsMap;

    void inlineCloneCalls(Function &F);
private:
    Module *M;
    std::string clonePrefixStr;
    std::vector<std::string> *skipSections;

    bool isInSkipSection(Function &F, std::vector<std::string> &sectionsList);
};

unsigned HybInlinePass::PassRunCount = 0;

HybInlinePass::HybInlinePass() : ModulePass(ID) {}

bool HybInlinePass::isInSkipSection(Function &F, std::vector<std::string> &sectionsList)
{
    for (unsigned i=0; i < sectionsList.size(); i++){
        if (0 == std::string(F.getSection()).compare(sectionsList[i])) {
            return true;
        }
    }
    return false;
}

/* Inline the calls to the clones in the normal path functions */
void HybInlinePass::inlineCloneCalls(Function &F)
{
    DEBUG(errs() << "inlineCloneCalls() on function: " << F.getName() << "\n");

    BasicBlock &EB = F.getEntryBlock();
    // There shall be an ICmpInst in the first basic block.
    ICmpInst *entryCmpInst = NULL;
    for (BasicBlock::iterator BI = EB.begin(), BE = EB.end(); BI != BE; BI++) {
        entryCmpInst = dyn_cast<ICmpInst>(&(*BI));
        if (NULL != entryCmpInst) break;
    }
    assert(NULL != entryCmpInst);

    // 1. Get the clone functions and their callsites
    SmallVector<Function *, 2> calledCloneFuncs(2);
    SmallVector<CallInst *, 2> callInstsToInline;
    std::string hybry_clone_1_tag = this->clonePrefixStr + HYBRY_CLONE_1_TAG;

    for (inst_iterator I = inst_begin(&F), E = inst_end(&F); I != E; I++) {
        CallInst *CI = dyn_cast<CallInst>(&(*I));
        if (NULL != CI) {
            Function *calledFunc = CI->getCalledFunction();
            assert(NULL != calledFunc);
            DEBUG(errs() << "callInst found: calling " << calledFunc->getName() << "()\n");
            if (std::string::npos == calledFunc->getName().find(clonePrefixStr)) {
                continue;
            }
            DEBUG(errs() << "\t>>> this is a call to cloned function" << "\n");

            if (std::string::npos != calledFunc->getName().find(hybry_clone_1_tag)){
                // call to clone1 function
		DEBUG(errs() << "\t>>>--- It's the clone 1\n");
                calledCloneFuncs[0] = calledFunc;
            } else {
		DEBUG(errs() << "\t>>>--- It's the clone 2\n");
                calledCloneFuncs[1] = calledFunc;
            }
            callInstsToInline.push_back(CI);
        }
    }
    DEBUG(errs() << "size of callInstsToInline : expected = 2, found = "
                 << callInstsToInline.size() << "\n");
    assert(2 == callInstsToInline.size() && "More than 2 call insts to inline, found");

    // 2. Mark to seggregate later from either clones by using markings from HybPrepPass
    SmallVector <std::vector<Value *>, 2> CIsOfCloneFuncs(2), LCsOfCloneFuncs(2), localsOfCloneFuncs(2);
    for (unsigned i=0; i < calledCloneFuncs.size(); i++) {
        // Function *cloneFunc = dyn_cast<Function>(calledCloneFuncs[i]);
        Function *cloneFunc = calledCloneFuncs[i];
        assert(NULL != cloneFunc);
        for (inst_iterator it = inst_begin(cloneFunc); it != inst_end(cloneFunc); it++) {
             Instruction *I = &(*it);
             CallInst *CI = dyn_cast<CallInst>(I);
             if (NULL != CI) {
                 if (0 != PassUtil::getAssignedID(CI, HYBPREP_NAMESPACE_LIBCALLS)) {
                     LCsOfCloneFuncs[i].push_back(CI);
                 } else if (0 != PassUtil::getAssignedID(CI, HYBPREP_NAMESPACE_CALLINSTS)) {
                     CIsOfCloneFuncs[i].push_back(CI);
                 }
             }
             if (0 != PassUtil::getAssignedID(&(*I), HYBPREP_NAMESPACE_LOCALS)) {
                 localsOfCloneFuncs[i].push_back(I);
             }
        }
    }
    assert(LCsOfCloneFuncs[0].size() == LCsOfCloneFuncs[1].size());
    assert(CIsOfCloneFuncs[0].size() == CIsOfCloneFuncs[1].size());
    assert(localsOfCloneFuncs[0].size() == localsOfCloneFuncs[1].size());
    for (int i=0; i < 2; i++) {
        PassUtil::assignIDs(*M, &(LCsOfCloneFuncs[i]),
                            std::string(HYBRY_NAMESPACE_LIBCALLS).append(std::to_string(i+1)));
        PassUtil::assignIDs(*M, &(CIsOfCloneFuncs[i]),
                            std::string(HYBRY_NAMESPACE_CALLINSTS).append(std::to_string(i+1)));
        PassUtil::assignIDs(*M, &(localsOfCloneFuncs[i]),
                            std::string(HYBRY_NAMESPACE_LOCALS).append(std::to_string(i+1)));
    }

    // 3. Inline the calls to cloned variants
    InlineFunctionInfo inlineFunctionInfo = InlineFunctionInfo(NULL);
    for (unsigned i=0; i < callInstsToInline.size(); i++) {
        InlineFunction(callInstsToInline[i], inlineFunctionInfo);
        NumCloneCallsInlined++;
    }

    return;
}

}
#endif
