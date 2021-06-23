#ifndef HYBRY_PASS_H
#define HYBRY_PASS_H

#if LLVM_VERSION >= 37
#ifndef DEBUG_TYPE
#define DEBUG_TYPE "hybrcvry"
#endif
#endif
#include <pass.h>
#include <common/tx_common.h>
#include <hybprep/HybPrepPass.h>
#include <ltckpt/ltCkptPass.h>
#include "HybRcvryCommon.h"
#include "RcvryHelper.h"
#include <llvm/Transforms/Utils/UnifyFunctionExitNodes.h>

#define HYBRY_TYPE_TSX				TX_TYPE_TSX
#define HYBRY_TYPE_LTCKPT			TX_TYPE_LTCKPT
#define HYBRY_GV_NAME_CURR_TX_TYPE		    "rcvry_current_tx_type"
#define HYBRY_GV_NAME_CURR_SITEID       	"rcvry_current_site_id"
#define HYBRY_GV_NAME_MAX_SITEID        	"rcvry_libcall_max_site_id"
#define HYBRY_GV_NAME_RETURN_VALUE      	"rcvry_return_value"
#define HYBRY_GV_NAME_RETURN_ADDR         	"rcvry_return_addr"
#define HYBRY_GV_NAME_GATES             	"rcvry_libcall_gates"
#define HYBRY_GV_NAME_SITES_INFO        	"rcvry_info"
#define HYBRY_NAMESPACE_TYPECHECKBR         "HYBRY_TXTYPECHECK"
#define RCVRY_FUNC_BBTRACING_FLUSH          "rcvry_prof_bbtrace_flush"

#define IS_INST_BEFORE(I1, I2)	({						                    \
        bool V = true;								                        \
        BasicBlock::iterator IT(I2);						                \
        for (BasicBlock::iterator it = IT, et = (I2)->getParent()->end();	\
             it != et; it++) {							                    \
            if ((I1) == &(*it)) {						                    \
		V = false; break;						                            \
            }									                            \
        }									                                \
        V;									                                \
        })

// Takes a vector V of instructions and arranges by order in SV
#define INST_SORT(V, SV)	({						                        \
	assert(0 != (V).size());						                        \
	assert(0 == (SV).size());						                        \
	BasicBlock *B = (V)[0]->getParent();					                \
        for (BasicBlock::iterator BI = B->begin(), BE = B->end(); BI != BE;	\
            BI++) {								                            \
            for (unsigned i=0; i < (V).size(); i++) {				        \
                if (&(*BI) == (V)[i]) {						                \
                    (SV).push_back((V)[i]);					                \
                }								                            \
            }									                            \
        }									                                \
        })

#define GET_THE_LOADINST(I)    ({						                    \
    BasicBlock *B = (I)->getParent();						                \
    BasicBlock::iterator it(I);							                    \
    LoadInst *T = NULL;								                        \
    for (; it != B->end(); it++) {						                    \
        T = dyn_cast<LoadInst>(&(*it));						                \
        if (NULL != T) break;							                    \
    }										                                \
    T;										                                \
    })

#define GET_THE_TOLINST(I)    ({						                    \
    BasicBlock *B = (I)->getParent();						                \
    BasicBlock::iterator it(I);							                    \
    CallInst *T = NULL;								                        \
    for (; it != B->end(); it++) {						                    \
        T = dyn_cast<CallInst>(&(*it));						                \
        if (NULL == T) continue;						                    \
        if (0 != PassUtil::getAssignedID(T, LTCKPT_NAMESPACE_TX_TYPE)) {    \
            break;								                            \
        }									                                \
    }										                                \
    T;										                                \
    })

using namespace llvm;

#define ADD_TO_ALLOCAS_CLONE_MAP(AI)        ({                              \
    Function *theFunc = (AI)->getParent()->getParent();                     \
    std::map<Function*, std::vector<AllocaInst*>* >::iterator MI, ME;       \
    std::vector<AllocaInst*> *allocaInsts = NULL;                           \
    switch (curr_cloneId) {                                                 \
        case 0:                                                             \
            MI = clone1NewAllocasMap.find(theFunc);                         \
            ME = clone1NewAllocasMap.end();                                 \
            if (MI == ME) {                                                 \
                allocaInsts = new std::vector<AllocaInst*>();               \
                allocaInsts->push_back((AI));                               \
                clone1NewAllocasMap.insert(std::make_pair(theFunc, allocaInsts));   \
            } else {                                                        \
                MI->second->push_back(AI);                                  \
            }                                                               \
            break;                                                          \
                                                                            \
        case 1:                                                             \
            MI = clone2NewAllocasMap.find(theFunc);                         \
            ME = clone2NewAllocasMap.end();                                 \
            if (MI == ME) {                                                 \
                allocaInsts = new std::vector<AllocaInst*>();               \
                allocaInsts->push_back((AI));                               \
                clone2NewAllocasMap.insert(std::make_pair(theFunc, allocaInsts));   \
            } else {                                                        \
                MI->second->push_back((AI));                                \
            }                                                               \
            break;                                                          \
                                                                            \
        case -1:                                                            \
        default:                                                            \
            errs() << "ERROR: curr_cloneId is -1 in Func: " << theFunc->getName() << "\n";   \
            break;                                                          \
    }                                                                       \
})

namespace llvm
{

STATISTIC(NumLibCalls,                  "Number of libcall callsites.");
STATISTIC(NumLibCallGatesAdded,         "..Number of libcalls surrounded with hybrcvry gates.");
STATISTIC(NumLibCallsSkipped,           "..Number of libcalls NOT surrounded with hybrcvry gates.");
STATISTIC(NumLibCallsNonFaultable, 	"....Number of libcalls that are 'non faultable', hence skipped.");
STATISTIC(NumLibCallsNoUsers, 		"....Number of libcalls with returns not used, hence skipped.");

typedef enum {
    /* Terminology:
     * A 'phi predecessor' typically leads to two 'phi blocks or phis' that lead to a 'phi successor'.
     * The 'phi successor' begins with a PHINode instruction.
     */
    HYBRY_SPLIT_IN_PHI_SUCCESSOR,
    HYBRY_SPLIT_IN_A_PHI,
    HYBRY_SPLIT_IN_NON_PHI,
    __NUM_SPLIT_TYPES
} split_type;

class HybRcvryPass : public ModulePass
{
public:
    static char ID;
    static unsigned PassRunCount;

    HybRcvryPass();
    virtual bool runOnModule(Module &M);
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

protected:
    std::map<CallInst *, CallInst *> libCallsMap;
    std::map<CallInst *, CallInst *> CIsMap;
    std::map<Value *, Value *> localsMap;
    std::map<Function *, std::vector<AllocaInst*>*> clone1NewAllocasMap, clone2NewAllocasMap;

    void getCloneArtifacts(Function &F,
                          std::map<CallInst*, CallInst*> &libCallsMap,
                          std::map<CallInst*, CallInst*> &callInstsMap,
                          std::map<Value*, Value*> &localsMap);
    bool rewireCallUsage(CallInst *CI, LoadInst **replacerInst, AllocaInst **allocaInst);
    bool replaceUsesWithAlloca(Instruction *I, LoadInst **replacerInst, AllocaInst **allocaInst);
    void replaceUsesWithAlloca(std::vector<Instruction*> &instructionsList);
    void replaceUsesWithAlloca(BasicBlock *prevBB, BasicBlock *targetBB);
    AllocaInst* rewireLibCallUsage(CallInst *libCallInst, LoadInst **replacerInst);
    void constructWindowEntryGatePair(uint64_t site_id,
                                      SmallVector <Instruction *, 2> insertBeforePair,
                                      std::vector <CallInst *> libCallsPair,
                                      std::vector<AllocaInst *> retValCtrlPair,
                                      BasicBlock * tsxBlock, BasicBlock *ltckptBlock,
                                      SmallVector<BasicBlock *, 2> &elseBlock,
                                      SmallVector<BasicBlock *, 2> &falseBlock);
    void constructWindowEntryGate(uint64_t site_id,
                                             std::vector<CallInst *> libCallsPair,
                                             std::vector<AllocaInst *> retValCtrlPair,
                                             SmallVector<BasicBlock *, 2> &thisBlock,
                                             SmallVector<BasicBlock *, 2> &libcallBlock,
                                             SmallVector<BasicBlock *, 2> &tolBlock);
    void fixupLibCallArgs(std::vector<CallInst *> libCallsPair,
                                    SmallVector<BasicBlock *, 2> libcallBlock,
                                    SmallVector<BasicBlock *, 2> thisBlock);
    void insertWindowEntryGates(Function &F);
    void insertWindowEntryGate(uint64_t site_id, std::vector<CallInst *> &libCallsPair,
                               std::map<CallInst *, AllocaInst*> &libCallsAllocaMap,
                               DominatorTree *DT, LoopInfo *LI);
    void insertTxTypeCheck(Instruction *insertBefore, BasicBlock *clone1BB,
                           BasicBlock *clone2BB);
    void insertTxTypeCheckAtReturnPoints(Function &F,
                          std::map<CallInst *, CallInst *> &F_CIsMap,
                          std::map<CallInst*, Instruction*> &CIReturnPointsMap);
    void mergeLocals(Function &F, std::map<Value *, Value *> &localVarsMap);
    void useFlexAllocasByRef(Function &F);
    StoreInst* getStoreInstOfThisLoadInst(LoadInst *LI, BasicBlock *BB);
    void adjustInterBlockDependencies(BasicBlock *predBB, BasicBlock *BB,
                                      std::vector<Instruction*> *dependencies=NULL,
                                      BasicBlock *destBB=NULL, DominatorTree *DT=NULL);
    split_type locateTheSplit(Instruction *splitPoint, PHINode **PHI=NULL);
    split_type getPHISafeSplitPoint(BasicBlock **bbToSplit, Instruction **splitPoint);
    void replaceWithDelayedFrees(Function &F);
    void fixupNewAllocaUsages(Function &F);

private:
    Module *M;
    GlobalVariable *libcallGatesGV;
    GlobalVariable *newReturnValueGV, *newReturnAddrGV;
    GlobalVariable *currSiteIdGV;
    GlobalVariable *rcvryInfoGV;
    GlobalVariable *currTxTypeGV;
    Function *saveRegsHook, *bbTraceFlushHook, *endOfWindowHook;
    std::vector<Value *> svRegsArgs;
    std::string clonePrefixStr, endOfWindowHookNameStr;
    std::vector<std::string> *skipSections, STMLibCalls;
    RcvryHelper *rcvryHelp;
    bool rcvryProfiling, rcvryBBTracing, faultInjection;
    std::map<Instruction*, AllocaInst*> tempInstAllocaReplacementMap;
    int curr_cloneId;

    void init();
    bool isInSkipSection(Function &F, std::vector<std::string> &sectionsList);
    bool isAUserOf(Instruction *I, Instruction *T);
    bool hasUsersIn(Instruction *I, BasicBlock *B);
    void getAllInstsThatBBDependsOn(BasicBlock *BB, std::vector<Instruction *> &BBDependsOnList);
    void addToListIfBBDependsOnInst(Instruction *I, BasicBlock *targetBB, std::vector<Instruction*> &BBDependsOnList,
                                                std::set<Instruction*> *uniquenessCheckSet);
    void addToListAllInterDependentInsts(std::vector<Instruction*> &BBDependsOnList);
    GetElementPtrInst* copyOverGEPBefore(Instruction *insertBefore, GetElementPtrInst *GEPInst);
    void createCopiesOfLoadDependencies(BasicBlock *BB, std::vector<Instruction*> &LoadsBBDependsOnList);
    Instruction* getValueInstInTheOtherClone(AllocaInst *theAlloca);
    AllocaInst* getCorrespondingAlloca(AllocaInst *theAlloca, std::vector<AllocaInst*> *fromAllocas);

};

unsigned HybRcvryPass::PassRunCount = 0;

HybRcvryPass::HybRcvryPass() : ModulePass(ID) {}

void HybRcvryPass::init()
{
    this->newReturnValueGV = this->M->getNamedGlobal(HYBRY_GV_NAME_RETURN_VALUE);
    assert(NULL != this->newReturnValueGV);
    this->newReturnAddrGV = this->M->getNamedGlobal(HYBRY_GV_NAME_RETURN_ADDR);
    assert(NULL != this->newReturnAddrGV);
    libcallGatesGV = this->M->getNamedGlobal(HYBRY_GV_NAME_GATES);
    assert(NULL != libcallGatesGV);
    currSiteIdGV = this->M->getNamedGlobal(HYBRY_GV_NAME_CURR_SITEID);
    assert(NULL != currSiteIdGV);
    currTxTypeGV = this->M->getNamedGlobal(HYBRY_GV_NAME_CURR_TX_TYPE);
    assert(NULL != currTxTypeGV);
    rcvryInfoGV = this->M->getNamedGlobal(HYBRY_GV_NAME_SITES_INFO);
    assert(NULL != rcvryInfoGV);

    Function *funcFuncType = NULL;
    svRegsArgs.clear();
    GlobalVariable *saveRegsBufferPtrGV = 
            this->M->getGlobalVariable(LTCKPT_ASM_SAVE_REGS_BUFFER);
    assert(NULL != saveRegsBufferPtrGV);
    svRegsArgs.push_back(saveRegsBufferPtrGV);
    svRegsArgs.push_back(CONSTANT_INT(*(this->M), 1));
    funcFuncType = this->M->getFunction(LTCKPT_ASM_SAVE_REGS_DUMMY_FUNC);
    assert(funcFuncType != NULL);
    FunctionType *saveRegsFuncType = funcFuncType->getFunctionType();
    Constant *saveRegsFunc =
            this->M->getOrInsertFunction(std::string(LTCKPT_ASM_SAVE_REGS_FUNC_NAME),
                                         saveRegsFuncType);
    saveRegsHook = cast<Function>(saveRegsFunc);
    assert(NULL != saveRegsHook);
    bbTraceFlushHook = (!rcvryBBTracing) ? NULL :
                       cast<Function>(this->M->getFunction(RCVRY_FUNC_BBTRACING_FLUSH));
    endOfWindowHook = ("" == endOfWindowHookNameStr) ? NULL :
                       cast<Function>(this->M->getFunction(endOfWindowHookNameStr));

    rcvryHelp = new RcvryHelper(this->M, rcvryInfoGV, endOfWindowHook, STMLibCalls);
}

bool HybRcvryPass::isInSkipSection(Function &F, std::vector<std::string> &sectionsList)
{
    for (unsigned i=0; i < sectionsList.size(); i++){
        if (0 == std::string(F.getSection()).compare(sectionsList[i])) {
            return true;
        }
    }
    return false;
}

/* Get the clone specific artifacts which the Inliner pass adds markings for. */
void HybRcvryPass::getCloneArtifacts(Function &F,
                                 std::map<CallInst*, CallInst*> &libCallsMap,
                                 std::map<CallInst*, CallInst*> &callInstsMap,
                                 std::map<Value*, Value*> &localsMap)
{
    DEBUG(errs() << "getCloneArtifacts() on function: " << F.getName() << "\n");

    BasicBlock &EB = F.getEntryBlock();
    // There shall be an ICmpInst in the first basic block.
    ICmpInst *entryCmpInst = NULL;
    for (BasicBlock::iterator BI = EB.begin(), BE = EB.end(); BI != BE; BI++) {
        entryCmpInst = dyn_cast<ICmpInst>(&(*BI));
        if (NULL != entryCmpInst) break;
    }
    assert(NULL != entryCmpInst);

    // 4. Place the inlined ones back into their respective clone buckets
    //    We can reuse the earlier buckets.
    SmallVector <std::vector<Value *>, 2> CIsOfCloneFuncs(2), LCsOfCloneFuncs(2), localsOfCloneFuncs(2);
    SmallVector <std::map<int, Value *>, 2> CIsOfCloneFuncsIDMap(2), LCsOfCloneFuncsIDMap(2), localsOfCloneFuncsIDMap(2);

    for (int i=0; i < 2; i++) {
        LCsOfCloneFuncs[i].clear(); CIsOfCloneFuncs[i].clear(); localsOfCloneFuncs[i].clear();
        LCsOfCloneFuncsIDMap[i].clear(); CIsOfCloneFuncsIDMap[i].clear(); localsOfCloneFuncsIDMap[i].clear();

        for (inst_iterator I = inst_begin(&F), E = inst_end(&F); I != E; I++) {
            if (false == (&(*I))->hasMetadataOtherThanDebugLoc()) continue;

            uint64_t id = 0;
            CallInst *CI = dyn_cast<CallInst>(&(*I));
            if (NULL != CI) {
                if (0 != (id = PassUtil::getAssignedID(CI,
                          std::string(HYBRY_NAMESPACE_LIBCALLS).append(std::to_string(i+1))))) {
                    LCsOfCloneFuncs[i].push_back(CI);
                    LCsOfCloneFuncsIDMap[i].insert(std::make_pair(id, CI));
                    DEBUG(errs() << "HYBRY_LIBCALL - assigned id: " << id << "\n");
                } else if (0 != (id = PassUtil::getAssignedID(CI,
                          std::string(HYBRY_NAMESPACE_CALLINSTS).append(std::to_string(i+1))))) {
                    CIsOfCloneFuncs[i].push_back(CI);
                    CIsOfCloneFuncsIDMap[i].insert(std::make_pair(id, CI));
                    DEBUG(errs() << "HYBRY_CALLINST - assigned id: " << id << "\n");
                }
            }
            if (0 != (id = PassUtil::getAssignedID(&(*I),
                          std::string(HYBRY_NAMESPACE_LOCALS).append(std::to_string(i+1))))) {
                    localsOfCloneFuncs[i].push_back(&(*I));
                    localsOfCloneFuncsIDMap[i].insert(std::make_pair(id, dyn_cast<Value>(&(*I))));
                    DEBUG(errs() << "HYBRY_LOCALS - assigned id: " << id << "\n");
            }
        }
    } // iteration for the two variants ends

    // 5. Arrange the elements that got inlined as counterparts
    //    Note that we use the HYBRY_NAMESPACE* here instead of the recently marked.
    for (unsigned i=0; i < LCsOfCloneFuncs[0].size(); i++) {
        CallInst *I1 = dyn_cast<CallInst>((LCsOfCloneFuncs[0])[i]);
        CallInst *I2 = NULL;
        uint64_t id1, id2;
        id1 = PassUtil::getAssignedID(I1,
                                      std::string(HYBRY_NAMESPACE_LIBCALLS).append(std::to_string(1)));
        I2 = dyn_cast<CallInst>(LCsOfCloneFuncsIDMap[1][id1]);
        assert(NULL != I2);
        id2 = PassUtil::getAssignedID(I2,
                                      std::string(HYBRY_NAMESPACE_LIBCALLS).append(std::to_string(2)));
        DEBUG(errs() << "HYBRY_LIBCALL - ID1: " << std::to_string(id1) << " ID2: "
                     << std::to_string(id2) << "\n");
        assert(id1 == id2);
        libCallsMap.insert(std::make_pair(I1, I2));

        // While we do so, let's fill in the libCallsMapCache too
        rcvryHelp->addLibCall(I1);
    }
    for (unsigned i=0; i < CIsOfCloneFuncsIDMap[0].size(); i++) {
        CallInst *I1 = dyn_cast<CallInst>(CIsOfCloneFuncs[0][i]);
        uint64_t id1 = PassUtil::getAssignedID(I1, HYBPREP_NAMESPACE_CALLINSTS);
        CallInst *I2 = dyn_cast<CallInst>(CIsOfCloneFuncsIDMap[1][id1]);
        assert(NULL != I2);
        assert(PassUtil::getAssignedID(I1, HYBPREP_NAMESPACE_CALLINSTS)
               == PassUtil::getAssignedID(I2, HYBPREP_NAMESPACE_CALLINSTS));
        callInstsMap.insert(std::make_pair(I1, I2));
    }
    for (unsigned i=0; i < localsOfCloneFuncsIDMap[0].size(); i++) {
        Value *I1 = localsOfCloneFuncs[0][i];
        uint64_t id1 = PassUtil::getAssignedID(I1, HYBPREP_NAMESPACE_LOCALS);
        Value *I2 = localsOfCloneFuncsIDMap[1][id1];
        assert(NULL != I2);
	DEBUG(errs() << "I1 ID: " << PassUtil::getAssignedID(I1, HYBPREP_NAMESPACE_LOCALS)
              << ", I2 ID: " << PassUtil::getAssignedID(I2, HYBPREP_NAMESPACE_LOCALS) << "\n");
        assert(PassUtil::getAssignedID(I1, HYBPREP_NAMESPACE_LOCALS)
               == PassUtil::getAssignedID(I2, HYBPREP_NAMESPACE_LOCALS));
        localsMap.insert(std::make_pair(I1, I2));
    } 
    return;
}

AllocaInst* HybRcvryPass::rewireLibCallUsage(CallInst *libCallInst,
                                          LoadInst **replacerInst)
{
    // Set the users of the libcall to instead use the global rcvry_return_value
    assert(NULL != libCallInst);
    assert(NULL != replacerInst);
    std::vector<User *> users;
    std::vector<Value *> args(1);
    if (TxUtil::isNonFaultableLibCall(libCallInst)) {
        // Let's remove its TOL as well!
        CallInst *tolInst = GET_THE_TOLINST(libCallInst);
        if (NULL != tolInst)
            tolInst->eraseFromParent();
        NumLibCallsNonFaultable++;
	    // For profiling, let's add call to a hook
        if (rcvryProfiling) {
            args[0] = CONSTANT_INT(*(this->M), PassUtil::getAssignedID(libCallInst, HYBPREP_NAMESPACE_ALL_LIBCALLS));
            PassUtil::createCallInstruction(rcvryHelp->countRcvryHitsSkippedNonFaultablesHook, args, "", libCallInst);
        }
        return NULL;
    }

    AllocaInst *retAllocaInst = NULL;
    if (false == rewireCallUsage(libCallInst, replacerInst, &retAllocaInst)) {
        NumLibCallsNoUsers++;
        if (rcvryProfiling) {
            args[0] = CONSTANT_INT(*(this->M),PassUtil::getAssignedID(libCallInst, HYBPREP_NAMESPACE_ALL_LIBCALLS));
            PassUtil::createCallInstruction(rcvryHelp->countRcvryHitsSkippedNoUsersHook, args, "", libCallInst);
        }
        return NULL;
    }

    // Rewiring successful at this point
    if (rcvryProfiling) {
        args[0] = CONSTANT_INT(*(this->M),PassUtil::getAssignedID(libCallInst, HYBPREP_NAMESPACE_ALL_LIBCALLS));
        PassUtil::createCallInstruction(rcvryHelp->countRcvryHitsProtectedWindowsHook, args, "", libCallInst);
    }
    return retAllocaInst;
}

bool HybRcvryPass::rewireCallUsage(CallInst *CI, LoadInst **replacerInst, AllocaInst **allocaInst)
{
    assert(NULL != CI && NULL != replacerInst && NULL != allocaInst);
    std::vector<User *> users;
    if (PassUtil::callHasNoUsers(CI, users)) {
        return false;    // No need to rewire
    }
    DEBUG(errs() << "Rewiring the uses of call: " << CI->getName() << "()'s return value.\n");

    // See if the first user is a store inst to an alloca. Then we must reuse the same alloca.
    StoreInst *CIStoreInst = dyn_cast<StoreInst>(users[0]);
    if (NULL != CIStoreInst) {
        AllocaInst *storeTarget = dyn_cast<AllocaInst>(CIStoreInst->getPointerOperand());
        if (NULL != storeTarget) {
            std::vector<User*> allocaUsers;
            PassUtil::getUsers(storeTarget, allocaUsers);
            assert(allocaUsers.size() > 0);
            LoadInst *loadTheAllocaInst = dyn_cast<LoadInst>(allocaUsers[0]);
            if (NULL != loadTheAllocaInst) {
                *allocaInst = storeTarget;
                *replacerInst = loadTheAllocaInst;
                return true;
            }
        }
    }

    Type *retType = dyn_cast<Value>(CI)->getType();
    if (NULL == *allocaInst) {
        Function *F = CI->getParent()->getParent();
        Instruction *allocaInsertPoint = F->getEntryBlock().getFirstNonPHI();
        *allocaInst = new AllocaInst(retType, Twine("rewireCI.alloca"), allocaInsertPoint);
        // ADD_TO_ALLOCAS_CLONE_MAP((*allocaInst));
    }
    assert(dyn_cast<Value>(*allocaInst)->getType()->isPointerTy());
    StoreInst *storeCItoAllocaInst __attribute__((unused))
                              = new StoreInst(CI, *allocaInst,
                                              "store callInst -> allocaInst",
                                               NEXT_INST(CI));
    // replacer inst that loads from the local variable
    *replacerInst = new LoadInst(retType, *allocaInst, Twine("load.alloca"),
                                 true, /* volatile = true */
                                 NEXT_INST(storeCItoAllocaInst));
    DEBUG(errs() << "\tReplacing its uses with local alloca: " << (*allocaInst)->getName() << "\n");
    for(unsigned i=0; i < users.size(); i++) {
        users[i]->replaceUsesOfWith(CI, *replacerInst);
    }
    return true;
}

void HybRcvryPass::replaceUsesWithAlloca(BasicBlock *prevBB, BasicBlock *targetBB)
{
    std::vector<User *> users;
    std::vector<Instruction*> directDeps;
    LoadInst *replacerInst;

    // Find all insts of prevBB that targetBB directly depends on.
    for (BasicBlock::iterator BI = prevBB->begin(), BE = prevBB->end(); BI != BE; BI++) {
        Instruction *I = &(*BI);
        PassUtil::getUsers(I, users);
        for (unsigned i=0; i < users.size(); i++) {
            if (dyn_cast<Instruction>(users[i])->getParent() == targetBB) {
                directDeps.push_back(I);
                break;
            }
        }
    }

    for (unsigned i=0; i < directDeps.size(); i++) {
        // just create a new alloca and store them there.
        // replace their uses in targetBB with a loadinst.
        Instruction *I = directDeps[i];
        Type *type = dyn_cast<Value>(I)->getType();
        Function *F = I->getParent()->getParent();
        Instruction *allocaInsertPoint = F->getEntryBlock().getFirstNonPHI();
        AllocaInst *allocaInst = NULL;
        LoadInst *LI = dyn_cast<LoadInst>(I);
        if (NULL != LI) {
            allocaInst = dyn_cast<AllocaInst>(LI->getPointerOperand());
        }
        if (NULL == allocaInst) {
            allocaInst = new AllocaInst(type, Twine("dep.alloca"), allocaInsertPoint);
            ADD_TO_ALLOCAS_CLONE_MAP(allocaInst);
            assert(dyn_cast<Value>(allocaInst)->getType()->isPointerTy());
            StoreInst *storeItoAllocaInst __attribute__((unused))
                                    = new StoreInst(I, allocaInst,
                                                    "store inst -> allocaInst",
                                                    NEXT_INST(I));
        }
        users.clear();
        PassUtil::getUsers(I, users);
        for (unsigned j=0; j < users.size(); j++) {
            if (dyn_cast<Instruction>(users[j])->getParent() == targetBB) {
                replacerInst = new LoadInst(type, allocaInst, Twine("load.dep.alloca"),
                                        true, /* volatile = true */
                                        dyn_cast<Instruction>(users[j]));
                users[j]->replaceUsesOfWith(I, replacerInst);
            }
        }
    }
    return;
}

bool HybRcvryPass::replaceUsesWithAlloca(Instruction *I, LoadInst **replacerInst, AllocaInst **allocaInst)
{
    assert(NULL != I && NULL != replacerInst && NULL != allocaInst);
    std::vector<User *> users;
        // Get all users
#if LLVM_VERSION >= 40
    users.assign(I->user_begin(), I->user_end());
#else
    users.assign(I->use_begin(), I->use_end());
#endif
    DEBUG(errs() << "Rewiring the uses of inst: " << I->getName() << "()'s return value.\n");
    if (0 == users.size()) {
        return true;
    }
    if (1 == users.size() && NULL != dyn_cast<LoadInst>(I)
        && (dyn_cast<Instruction>(users[0])->getParent() == I->getParent())) {
        *replacerInst = dyn_cast<LoadInst>(I);
        return true;
    }

    Type *type = dyn_cast<Value>(I)->getType();
    if (NULL == *allocaInst) {
        Function *F = I->getParent()->getParent();
        Instruction *allocaInsertPoint = F->getEntryBlock().getFirstNonPHI();
        *allocaInst = new AllocaInst(type, Twine("nonLoadGEP.alloca"), allocaInsertPoint);
        ADD_TO_ALLOCAS_CLONE_MAP(*allocaInst);
    }
    assert(dyn_cast<Value>(*allocaInst)->getType()->isPointerTy());
    StoreInst *storeItoAllocaInst __attribute__((unused))
                              = new StoreInst(I, *allocaInst,
                                              "store inst -> allocaInst",
                                               NEXT_INST(I));

    DEBUG(errs() << "\tReplacing its uses with local alloca: " << (*allocaInst)->getName() << "\n");
    for(unsigned i=0; i < users.size(); i++) {
        // replacer inst that loads from the local variable
        *replacerInst = new LoadInst(type, *allocaInst, Twine("load.alloca"),
                                        true, /* volatile = true */
                                        dyn_cast<Instruction>(users[i]));
        users[i]->replaceUsesOfWith(I, *replacerInst);
    }
    return true;
}

void HybRcvryPass::replaceUsesWithAlloca(std::vector<Instruction*> &instructionsList) {
    // Replace uses of non-LoadInst and non-GEPs with allocas
    for (unsigned i=0; i < instructionsList.size(); i++) {
        Instruction *I = instructionsList[i];
        if ( NULL != dyn_cast<LoadInst>(I)
             || NULL != dyn_cast<AllocaInst>(I)) {
                 continue;
        }
        // non-LoadInst and non-GEPs:
        AllocaInst *allocaInst = NULL;
        LoadInst *replacerInst = NULL;
        std::map<Instruction*, AllocaInst*>::iterator itInstAlloca;
        if (tempInstAllocaReplacementMap.end() != (itInstAlloca = tempInstAllocaReplacementMap.find(I))) {
            allocaInst = itInstAlloca->second;
        }
        assert(true == replaceUsesWithAlloca(I, &replacerInst, &allocaInst));
    }
    return;
}

void HybRcvryPass::replaceWithDelayedFrees(Function &F)
{
    static Constant *delayedFreeFunc = this->M->getFunction("rcvry_ra_delayed_free");
    static Function *delayedFreeHook = cast<Function>(delayedFreeFunc);

    assert(NULL != delayedFreeHook);

    DEBUG(errs() << "rwdf: Function: " << F.getName() << "\n");
    std::vector<CallInst*> callsToRemove;
    for (inst_iterator II = inst_begin(&F), E = inst_end(&F); II != E; II++) {
        CallInst *callInst = dyn_cast<CallInst>(&(*II));
        std::vector<Value *> args;
        args.clear();
        if (NULL != callInst) {
            if (!PassUtil::isLibCallInst(callInst)) {
                continue;
            }
            std::string libFuncName = callInst->getCalledFunction()->getName();
            if (0 == libFuncName.compare("free")) {
                // replace callInst with call to delayed free
                CallSite callsite = CallSite::CallSite(callInst);
                args.push_back(callsite.getArgument(0));
                PassUtil::createCallInstruction(delayedFreeHook, args, "", callInst);
                callsToRemove.push_back(callInst);
            }
        }
    }

    for (unsigned i=0; i < callsToRemove.size(); i++) {
        callsToRemove[i]->eraseFromParent();
    }
}

void HybRcvryPass::insertWindowEntryGates(Function &F)
{
#if LLVM_VERSION >= 37
    DominatorTree *DT = &getAnalysis<DominatorTreeWrapperPass>(F).getDomTree();
#else
    DominatorTree *DT = &getAnalysis<DominatorTree>(F);
#endif
    LoopInfo *LI = &getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();

    std::map<CallInst *, AllocaInst *> libCallsAllocaMap;
    std::map<CallInst *, CallInst *> F_libCallsMap, F_CIsMap;
    std::map<Value *, Value *> F_localsMap;

    tempInstAllocaReplacementMap.clear();

#ifdef RCVRY_ENABLE_DELAYED_FREES
    replaceWithDelayedFrees(F);
#endif

    // First, inline the calls to cloned variants
    getCloneArtifacts(F, F_libCallsMap, F_CIsMap, F_localsMap);
    DEBUG(errs() << "Fetched inlined clone artifacts.\n");

    libCallsMap.insert(F_libCallsMap.begin(), F_libCallsMap.end());
    CIsMap.insert(F_CIsMap.begin(), F_CIsMap.end());
    localsMap.insert(F_localsMap.begin(), F_localsMap.end());

    // rewire all libcall usages
    NumLibCalls += (F_libCallsMap.size() * 2);
    for (std::map<CallInst*, CallInst*>::iterator it = F_libCallsMap.begin();
         it != F_libCallsMap.end(); it++) {
        SmallVector<CallInst *, 2> LCs(2);
        SmallVector<AllocaInst *, 2> LCAllocas(2);
        SmallVector<LoadInst *, 2> LCReplacers(2);

        LCs[0] = (*it).first; LCs[1] = (*it).second;

        for(int i=0; i < 2; i++) {
            curr_cloneId = i;
            LCAllocas[i] = rewireLibCallUsage(LCs[i], &(LCReplacers[i]));
            if (NULL == LCAllocas[i]) { /* LCs that have no users (including void return types) */
                NumLibCallsSkipped++;
                continue;
            }
            libCallsAllocaMap.insert(std::make_pair(LCs[i], LCAllocas[i]));

            // Also inject fault right before the library call for fault-injection experiments
            if (this->faultInjection) {
                std::vector<Value *> args(1);
                args[0] = CONSTANT_INT(*(this->M), PassUtil::getAssignedID(LCs[i], HYBPREP_NAMESPACE_ALL_LIBCALLS));
                assert(NULL != rcvryHelp->fiHook);
                PassUtil::createCallInstruction(rcvryHelp->fiHook, args, "", LCs[i]);
            }
            if (NULL != this->endOfWindowHook) {
                std::vector<Value *> args;
                PassUtil::createCallInstruction(this->endOfWindowHook, args, "", LCs[i]);
            }
        }
        curr_cloneId = -1;
    }
    DEBUG(errs() << "rewiring libcall usage done.\n");

    // rewire all call usages, so that we can add txtype check at the return points
    DEBUG(errs() << "Num CIs: " << F_CIsMap.size() << "\n");
    DEBUG(errs() << "Rewiring all call usages first.\n");
    std::map<CallInst*, Instruction*> CIReturnPointsMap;
    for (std::map<CallInst*, CallInst*>::iterator it = F_CIsMap.begin();
         it != F_CIsMap.end(); it++) {
        SmallVector<CallInst *, 2> CIs(2);
        CIs[0] = (*it).first; CIs[1] = (*it).second;
        std::vector<AllocaInst*> CIRetAllocas[2];
        for(int i=0; i < 2; i++) {
            curr_cloneId = i;
            Instruction *returnPoint;
            AllocaInst *CIRetAlloca = NULL;
            LoadInst *loadCIRet = NULL;
            if (rewireCallUsage(CIs[i], &loadCIRet, &CIRetAlloca)) {
                returnPoint = loadCIRet;
                CIRetAllocas[i].push_back(CIRetAlloca);
            } else {
                returnPoint = CIs[i];
            }
            CIReturnPointsMap.insert(std::make_pair(CIs[i], returnPoint));
        }
        curr_cloneId = -1;
        assert(CIRetAllocas[0].size() == CIRetAllocas[1].size());
        // Append the newly added allocas to the locals list.
        for (unsigned i=0; i < CIRetAllocas[1].size(); i++) {
            F_localsMap.insert(std::make_pair(CIRetAllocas[0][i], CIRetAllocas[1][i]));
        }
    }

    /* Problem!! At this point, the two sets of inlined clones will be using their
     * own sets of local variables.
     *
     * Merge the local variable usages, so that local state gets shared between the 
     * two variants of the function (which have been inlined)  
     */
#ifndef HYB_NO_MERGE_LOCALS
    mergeLocals(F, F_localsMap);
#endif

    /* For each clone1 libcall, do:
     * 1. insert ICmp <gates[site_id]>, TX_TYPE_TSX
     * 2. insert branch <BB of clone1 LC>, ICmp val, <else basic block>
     * 3. In <else basic block>,
     *      insert ICmp <gates[site_id], TX_TYPE_LTCKPT
     *      insert branch <BB of clone2 LC>, ICmp val, <false block>
     * 4. prepare the false block
     */

    for (std::map<CallInst*, CallInst*>::iterator it = F_libCallsMap.begin();
         it != F_libCallsMap.end(); it++) {
        SmallVector<CallInst *, 2> LCs(2);
        LCs[0] = (*it).first; LCs[1] = (*it).second;

        assert(LCs[0] != LCs[1]);
        // Skip those where we cannot recover from. Adding gates is not going to
        // be of any use there.
        if (libCallsAllocaMap.end() == libCallsAllocaMap.find(LCs[0])) {
            continue;
        }
        uint64_t site_id = PassUtil::getAssignedID(LCs[0], HYBPREP_NAMESPACE_ALL_LIBCALLS);
        DEBUG(errs() << "Inserting window entry gate at site: " << site_id << "\n");
        std::vector<CallInst *> LCpair(LCs.begin(), LCs.end());
        insertWindowEntryGate(site_id, LCpair, libCallsAllocaMap, DT, LI);
    }

#ifndef HYB_PERFDBG_NO_CHECKS_AFTER_RETS	// For perf-debugging only
    insertTxTypeCheckAtReturnPoints(F, F_CIsMap, CIReturnPointsMap);
#endif
    useFlexAllocasByRef(F);
    return;
}

void HybRcvryPass::constructWindowEntryGate(uint64_t site_id,
                                             std::vector<CallInst *> libCallsPair,
                                             std::vector<AllocaInst *> retValCtrlPair,
                                             SmallVector<BasicBlock *, 2> &thisBlock,
                                             SmallVector<BasicBlock *, 2> &libcallBlock,
                                             SmallVector<BasicBlock *, 2> &tolBlock)
{
    /* A window entry gate constitutes the following:
     * 1. GEPInst to access the global gate array
     * 2. LoadInst that fetches gate value
     * 3. ICmpInst that compares the gate value
     * 4. BranchInst that forks the flow into TSX and LTCKPT paths
     * 5. ElseBlock that leads towards LTCKPT and recovery paths
     * 6. Accessories:
     *    a. StoreInst to set site_id variable
     *    b. Setting recovery action for the site
     *    c. Saving register state
     */

    // 1. GEPInst
    Function __attribute__((unused)) *F = libcallBlock[0]->getParent();
    std::vector<Value*> indices;
    indices.push_back(ZERO_CONSTANT_INT(*M));
    indices.push_back(CONSTANT_INT(*M, site_id));
    // Instruction *GEPInsertPt = F->getEntryBlock().getFirstNonPHI();

    Instruction *GEPInst = NULL;

    // Everything else is in pairs # Nope!
    LoadInst *gateVal;
    ICmpInst *firstICmp;
    Instruction *gateInsertPt;
    SmallVector<BranchInst *, 2> firstBrInst(2);
    BranchInst *secondBrInst;
    StoreInst *storeSiteId;
    CallInst *saveRegs;
    static const std::vector<tx_type_t> txType(HYBRY_TYPE_TSX, HYBRY_TYPE_LTCKPT);

    for (int i=0; i < 2; i++) {
        curr_cloneId = i;
        Instruction *thisBlockTerminator = thisBlock[i]->getTerminator();
        firstBrInst[i] = BranchInst::Create(libcallBlock[0], thisBlock[i]);
        thisBlockTerminator->eraseFromParent();
    }
    curr_cloneId = -1;

    gateInsertPt = libcallBlock[0]->getTerminator();
    GEPInst = PassUtil::createGetElementPtrInstruction(libcallGatesGV, indices,
                                                                    "", gateInsertPt);
    assert(GEPInst && "Failed creating GEP instruction accessing libcallGatesGV.");
    gateVal = new LoadInst(GEPInst, "", false, gateInsertPt); // after the libcall
    firstICmp = new ICmpInst(gateInsertPt, ICmpInst::ICMP_EQ, gateVal,
                                CONSTANT_INT(*M, HYBRY_TYPE_TSX),
                                std::string("icmp.window_entry"));
    secondBrInst = BranchInst::Create(tolBlock[0], tolBlock[1], firstICmp, gateInsertPt);
    PassUtil::setBranchWeight(*M, secondBrInst, 65536, 1);
    gateInsertPt->eraseFromParent();

    fixupLibCallArgs(libCallsPair, libcallBlock, thisBlock);

    // Accessories:
    Instruction *insertBefore = GEPInst;

    // Assumption: We have already merged the arguments to the two libcalls, so that
    // we can simply discard one libcallBlock

    // Insert recovery action settings right at the end of each of the two thisBlocks, before they
    // merge into the libcallBlock that has a gate to deviate into respective TOL blocks.
    for (int i=0; i < 1; i++) {
        rcvryHelp->setRecoveryAction(site_id, TX_TYPE_INVALID, libCallsPair[0],
                                     retValCtrlPair[0], GEPInst, rcvryProfiling);
    }

    StoreInst __attribute__((unused)) *storeRetToGV
                = libCallsPair[0]->getType() != newReturnValueGV->getType()
                                        ? new StoreInst(RCVRY_TO_VOID_PTR(libCallsPair[0], insertBefore),
                                                        newReturnValueGV, insertBefore)
                                        : new StoreInst(libCallsPair[0], newReturnValueGV, insertBefore);
    storeSiteId = new StoreInst(CONSTANT_INT(*M, site_id), currSiteIdGV, insertBefore);
    saveRegs = PassUtil::createCallInstruction(saveRegsHook, svRegsArgs, "", insertBefore);

    LoadInst *loadFromReturnValueGV = new LoadInst(newReturnValueGV, "", true, insertBefore);
    StoreInst *storeToRetValCtrl1 = NULL, *storeToRetValCtrl2 = NULL;

    if (libCallsPair[0]->getType()->isPointerTy()) {
        BitCastInst *bitCastReturnValueGV = new BitCastInst(loadFromReturnValueGV,
                                                 libCallsPair[0]->getType(),
                                                 "bitcast rcvry_return_value",
                                                 insertBefore);
        storeToRetValCtrl1 = new StoreInst(bitCastReturnValueGV, retValCtrlPair[0], insertBefore);
        storeToRetValCtrl2 = new StoreInst(bitCastReturnValueGV, retValCtrlPair[1], insertBefore);
    } else {
        PtrToIntInst *voidPtr2IntReturnValueGV = new PtrToIntInst(loadFromReturnValueGV,
                                                  libCallsPair[0]->getType(),
                                                  "ptr2int rcvry_return_value",
                                                  insertBefore);
        storeToRetValCtrl1 = new StoreInst(voidPtr2IntReturnValueGV, retValCtrlPair[0], insertBefore);
        storeToRetValCtrl2 = new StoreInst(voidPtr2IntReturnValueGV, retValCtrlPair[1], insertBefore);
    }
    return;
}

void HybRcvryPass::fixupLibCallArgs(std::vector<CallInst *> libCallsPair,
                                    SmallVector<BasicBlock *, 2> libcallBlock,
                                    SmallVector<BasicBlock *, 2> thisBlock)
{
    std::vector<Value*> libcall1Args(libCallsPair[0]->arg_begin(), libCallsPair[0]->arg_end());
    std::vector<Value*> libcall2Args(libCallsPair[1]->arg_begin(), libCallsPair[1]->arg_end());
    std::vector<AllocaInst*> argsAllocas;
    Function *F = libCallsPair[0]->getParent()->getParent();

    assert(libcall1Args.size() == libcall2Args.size());

    for (unsigned i=0; i < libcall1Args.size(); i++) {
        if (libcall1Args[i] == libcall2Args[i]) {
            continue;
        }

        AllocaInst *alloca = new AllocaInst(libcall1Args[i]->getType(), "arg.alloca",
                                                F->getEntryBlock().getFirstNonPHI());
        ADD_TO_ALLOCAS_CLONE_MAP(alloca);

        if (NULL != dyn_cast<Instruction>(libcall1Args[i])) {
                dyn_cast<Instruction>(libcall1Args[i])->moveBefore(thisBlock[0]->getTerminator());
                dyn_cast<Instruction>(libcall2Args[i])->moveBefore(thisBlock[1]->getTerminator());
        }
        StoreInst __attribute__((unused)) *store1, *store2;
        store1 = new StoreInst(libcall1Args[i], alloca, "store.1.(load)arg", thisBlock[0]->getTerminator());
        store2 = new StoreInst(libcall2Args[i], alloca, "store.2.(load)arg", thisBlock[1]->getTerminator());
        LoadInst *loadAlloca = new LoadInst(alloca, "load.arg", false, libCallsPair[0]);
        libCallsPair[0]->replaceUsesOfWith(libcall1Args[i], loadAlloca);
    }
    return;
}

/* Inserts entry gates at the start of every libcall-interval (aka recovery window).
 * Parameters:
 *     site_id		    : libcall site_id. Note that it is the same for both in the pair.
 *     libCallsPair     : pair of libcalls from clone1, clone2, correspondingly
 *     libCallsAllocaMap: allocas corresponding to every libcall, obtained after rewiring.
 */
void HybRcvryPass::insertWindowEntryGate(uint64_t site_id, std::vector<CallInst *> &libCallsPair,
                                      std::map<CallInst *, AllocaInst*> &libCallsAllocaMap,
                                      DominatorTree *DT, LoopInfo *LI)
{
    // 0. Sanity check
    assert(2 == libCallsPair.size());
    /* Those with void return types and those that have no users
     * We cannot recover using these libcall points. Eg., printf() statements.
     * Essentially all libcall intervals that start with such libcalls are doomed anyway.
     */
    assert (NULL != libCallsAllocaMap[libCallsPair[0]]);

    Function *F = libCallsPair[0]->getParent()->getParent();
    assert(F == libCallsPair[1]->getParent()->getParent());

    /* if profiling-bbtrace is enabled, insert the flush func() for prev window trace before
     * the libcall */
    if (rcvryProfiling && rcvryBBTracing) {
         std::vector<Value*> args;
         assert(NULL != bbTraceFlushHook);
         for (int i=0; i < 2; i++) {
            curr_cloneId = i;
            PassUtil::createCallInstruction(bbTraceFlushHook, args, "", libCallsPair[i]);
         }
    }
    curr_cloneId = -1;

    /* Right after the libcall, add a store inst to assign rcvry_return_addr to the allocaInst */
    for (int i=0; i < 2; i++) {
        curr_cloneId = i;
        // Instruction *nextInst = NEXT_INST(libCallsPair[i]);
        Value *alloca = RCVRY_TO_VOID_PTR(dyn_cast<Value>(libCallsAllocaMap[libCallsPair[i]]), libCallsPair[i]);
        new StoreInst(alloca, newReturnAddrGV, true, libCallsPair[i]);
    }
    curr_cloneId = -1;

    SmallVector <BasicBlock *, 2> thisBlock(2), thatBlock(2), libcallBlock(2), tolBlock(2);
    SmallVector <Instruction *, 2> firstSplitPoint(2), secondSplitPoint(2);
    SmallVector <Instruction *, 2> firstBranchPoint(2), thirdBranchPoint(2);

    for (int i=0; i < 2; i++) {
        curr_cloneId = i;
        thisBlock[i] = libCallsPair[i]->getParent();
    }
    curr_cloneId = -1;

    /* 1. The first split:
     *    [<thisBlock> .. libcall; tol ] ==> [<thisBlock> .. ] [<libcallBlock> libcall; TOL; <rest> ]
     *
     * Note:  When splitting, specified inst goes into the the latter block
     */
    split_type splitTy;
    for (int i=0; i < 2; i++) {
        curr_cloneId = i;
        firstSplitPoint[i] = dyn_cast<Instruction>(libCallsPair[i]);
        DEBUG(errs() << "[before] splitpoint inst: " << firstSplitPoint[i]->getName() << "\n");

        // It's not desirable to have a phi node at the split point.
        // If that's the case, we should split before the branch that leads to the phi.
        splitTy = getPHISafeSplitPoint(&(thisBlock[i]), &(firstSplitPoint[i]));
        DEBUG(errs() << "[after] splitpoint inst: " << firstSplitPoint[i]->getName() << "\n");

        // Split.
        thatBlock[i] = SplitBlock(thisBlock[i], firstSplitPoint[i], DT, LI);
        firstBranchPoint[i] = thisBlock[i]->getTerminator();
        libcallBlock[i] = libCallsPair[i]->getParent();

        // Adjust the dependencies between the newly split blocks, eg., its arg loads, etc
        adjustInterBlockDependencies(thisBlock[i], thatBlock[i]);
    }
    curr_cloneId = -1;
    DEBUG(errs() << "First split [done]\n");

    /* 3. Construct the gate elements that constitutes branches 
     *    that provide deviation towards ltckpt and errpath paths, respectively
     *    Like:
     *    if (tsx) { this; } else { if (ltckpt) { that; } else { falseblock; } }
     */
    SmallVector <BasicBlock *, 2> elseBlock(2), falseBlock(2);
    std::vector <AllocaInst *> retValCtrls(2);
    retValCtrls[0] = libCallsAllocaMap[libCallsPair[0]];
    retValCtrls[1] = libCallsAllocaMap[libCallsPair[1]];

    /* 4. The second split:
     *    [<libcallBlock> libcall; store; tol; rest ] ==> [<libcallblock> libcall; store; ] [<tolBlock> tol; rest ]
     */
    DEBUG(errs() << "Going for second split...\n");
    for (int i=0; i < 2; i++) {
        curr_cloneId = i;
        LoadInst *loadLC = NULL;
        loadLC = GET_THE_LOADINST(libCallsPair[i]);
        assert(NULL != loadLC);
        secondSplitPoint[i] = dyn_cast<Instruction>(loadLC);
        assert(NULL != secondSplitPoint[i]);
        thatBlock[i] = libcallBlock[i];
        tolBlock[i] = SplitBlock(thatBlock[i], secondSplitPoint[i], DT, LI);

        // ensure the load of the alloca happens after the call to tol
        Instruction *I1 = tolBlock[i]->getFirstNonPHI();
        Instruction *I2 = NEXT_INST(I1);
        LoadInst *LI1 = dyn_cast<LoadInst>(I1);
        CallInst *CI2 = dyn_cast<CallInst>(I2);
        if ((NULL != LI1) && (NULL != CI2)) {
            // CI2 shall be the TOL. Let's move LI1 after CI2
            if (std::string::npos != CI2->getCalledFunction()->getName().find("top_of_the_loop")) {
                I2->moveBefore(I1);
            }
        }

        adjustInterBlockDependencies(thatBlock[i], tolBlock[i]);
    }
    curr_cloneId = -1;

    constructWindowEntryGate(site_id, libCallsPair, retValCtrls, thisBlock, thatBlock, tolBlock);    
    NumLibCallGatesAdded += 1;  // accounting for gates added for both the variants
    return;
}

void HybRcvryPass::insertTxTypeCheck(Instruction *insertBefore, BasicBlock *clone1BB,
                                  BasicBlock *clone2BB)
{
    assert(NULL != insertBefore);
    assert(NULL != clone1BB);
    assert(NULL != clone2BB);
    assert(clone1BB != clone2BB);
    // Check if we already have a TxTypeCheck here. If so, dont add one more
    BranchInst *existingBrInst = dyn_cast<BranchInst>(insertBefore);
    if (NULL != existingBrInst) {
        if (existingBrInst->hasMetadataOtherThanDebugLoc()) {
            // Is it something we have added?
            if (NULL != existingBrInst->getMetadata(HYBRY_NAMESPACE_TYPECHECKBR)) {
                DEBUG(errs() << "There is already a txtype check branch here. No need to add one more.\n");
                return;
            }
        }
    }
    LoadInst *loadTxType = new LoadInst(currTxTypeGV, "", false, insertBefore);
    ICmpInst *iCmp = new ICmpInst(insertBefore, ICmpInst::ICMP_EQ, loadTxType,
                                   CONSTANT_INT(*M, HYBRY_TYPE_TSX),
                                   "icmp.txTypeCheck");
    BranchInst *branch = BranchInst::Create(clone1BB, clone2BB, iCmp, insertBefore);
    PassUtil::setBranchWeight(*M, branch, 65536, 1);
    branch->setMetadata(HYBRY_NAMESPACE_TYPECHECKBR, PassUtil::createMDNodeForConstant(this->M, 0));
    return;
}

void HybRcvryPass::insertTxTypeCheckAtReturnPoints(Function &F,
                                                std::map<CallInst *, CallInst *> &F_CIsMap,
                                                std::map<CallInst*, Instruction*> &CIReturnPointsMap)
{
#if LLVM_VERSION >= 37
    DominatorTree *DT = &getAnalysis<DominatorTreeWrapperPass>(F).getDomTree();
#else
    DominatorTree *DT = &getAnalysis<DominatorTree>(F);
#endif
    LoopInfo *LI = &getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();
    
    DEBUG(errs() << "Inserting TxTypeCheck at Return Points for function " << F.getName() << "\n");
    for (std::map<CallInst*, CallInst*>::iterator it = F_CIsMap.begin();
         it != F_CIsMap.end(); it++) {
        SmallVector<CallInst *, 2> CIs(2);
        SmallVector<BasicBlock *, 2> BBs(2), nextBBs(2);
        SmallVector<Instruction *, 2> branchPoint(2);
        std::vector<Instruction *> usersChain;
        CIs[0] = (*it).first; CIs[1] = (*it).second;

        // split the bb after the callinst
        // [<BBs> callInst; nextInst ] ==> [<BBs> callInst; ] [<nextBBs> nextInst ]
        for (int i=0; i < 2; i++) {
            curr_cloneId = i;
            Instruction *splitPoint = NEXT_INST(CIReturnPointsMap[CIs[i]]);
            assert(NULL != splitPoint);
            split_type splitType = locateTheSplit(splitPoint);
            BasicBlock *phiSucc = NULL;

            switch (splitType) {
                case HYBRY_SPLIT_IN_PHI_SUCCESSOR:
                    phiSucc = splitPoint->getParent();
                    break;

                case HYBRY_SPLIT_IN_A_PHI:
                    phiSucc = (splitPoint->getParent())->getSingleSuccessor();
                    assert(NULL != phiSucc);

                    // Split after the phiSucc
                    if (phiSucc != splitPoint->getParent()) {
                        splitPoint = phiSucc->getFirstNonPHI();
                    }
                    break;

                case HYBRY_SPLIT_IN_NON_PHI:
                default:
                    break;
            };

            BBs[i] = splitPoint->getParent();
            BranchInst *br = dyn_cast<BranchInst>(splitPoint); 
            if ((NULL != br) && br->isConditional()
                && (NULL != dyn_cast<PHINode>(br->getCondition()))) {
                nextBBs[i] = NULL;
            } else {
                // Note: The splitPoint instruction goes to the new basic block
                nextBBs[i] = SplitBlock(BBs[i], splitPoint, DT, LI);
                branchPoint[i] = BBs[i]->getTerminator();
                if (splitType != HYBRY_SPLIT_IN_NON_PHI) {
                    adjustInterBlockDependencies(BBs[i], CIs[i]->getParent(), NULL, nextBBs[i], DT);
                } else {
                    adjustInterBlockDependencies(BBs[i], nextBBs[i], NULL, NULL, DT);
                }
            }
        }
        curr_cloneId = -1;

        // add checks connecting them together
        if (NULL == nextBBs[0]) {
            DEBUG(errs() << "nextBBs[0] is NULL. Unconditional branching case.\n");
            BranchInst *br0 = dyn_cast<BranchInst>(BBs[0]->getTerminator());
            BranchInst *br1 = dyn_cast<BranchInst>(BBs[1]->getTerminator());
            for (unsigned j=0; j < br0->getNumSuccessors(); j++) {
                BasicBlock *thatBB0 = SplitBlock(br0->getSuccessor(j),
                                                br0->getSuccessor(j)->getFirstNonPHI());
                Instruction *br0SuccTerm = br0->getSuccessor(j)->getTerminator();
                BasicBlock *thatBB1 = SplitBlock(br1->getSuccessor(j),
                                                br1->getSuccessor(j)->getFirstNonPHI());
                Instruction *br1SuccTerm = br1->getSuccessor(j)->getTerminator();

                insertTxTypeCheck(br0->getSuccessor(j)->getFirstNonPHI(),
                                  thatBB0, thatBB1);
                insertTxTypeCheck(br1->getSuccessor(j)->getFirstNonPHI(),
                                  thatBB0, thatBB1);
                br0SuccTerm->eraseFromParent();
                br1SuccTerm->eraseFromParent();
            }
            continue;
        }

        for (int i=0; i < 2; i++) {
            curr_cloneId = i;
            insertTxTypeCheck(BBs[i]->getTerminator(), nextBBs[0], nextBBs[1]);
            branchPoint[i]->eraseFromParent();
        }
        curr_cloneId = -1;
    }
    return;
}

/* On normal functions */
void HybRcvryPass::mergeLocals(Function &F, std::map<Value *, Value *> &localVarsMap)
{
    for (std::map<Value*, Value*>::iterator it = localVarsMap.begin();
         it != localVarsMap.end(); it++) {
        SmallVector<Instruction *, 2> locals(2);
        locals[0] = dyn_cast<Instruction>((*it).first);
        locals[1] = dyn_cast<Instruction>((*it).second);
        
        Type *type1 = locals[0]->getType();
        Type *type2 = locals[1]->getType();
        assert(type1 == type2);

        std::vector<User*> users1, users2;
#if LLVM_VERSION >= 40
        users2.assign(locals[1]->user_begin(), locals[1]->user_end());
#else
        users2.assign(locals[1]->use_begin(), locals[1]->use_end());
#endif

        /* Instructions shall be of the form:
         * S: store P, V
         * L: load allocaX
         * A: alloca Y
         * R: retval = call func
         * 
         * Transformations:
         * S': [insert allocaP]; [transform both]  store allocaP, V
         * L': replace the other to use the same allocaX
         * A': replace uses of the other alloca with this one
         * R': use the rewireCallUsage() function - this is already done anyway
         *
         * Note: Now, if we just perform A to A', then all others get automatically done.
         */
        AllocaInst *alloca = dyn_cast<AllocaInst>(locals[0]);
        if (NULL != alloca) {
            /* Replace uses of locals[1] with locals[0].
             * Either way will do, however in the end both variants must
             * use the same alloca
             */
            assert(alloca->getAllocatedType() == (dyn_cast<AllocaInst>(locals[1]))->getAllocatedType());
            for (unsigned i=0; i < users2.size(); i++) {
                /* If the alloca and its use is within the same basic block,
                   we dont want to disturb it, as def and use remain contained within.
                 */
                Instruction *inst = dyn_cast<Instruction>(users2[i]);
                assert(NULL != inst);
                if (inst->getParent() != locals[1]->getParent()) {
                    users2[i]->replaceUsesOfWith(locals[1], locals[0]);
                }
            }
        }
    } // for ends.
    return;
}

// T is a user of I? 
bool HybRcvryPass::isAUserOf(Instruction *I, Instruction *T)
{
    assert(NULL != I && NULL != T);
    std::vector<User *> users;
#if LLVM_VERSION >= 40
    users.assign(I->user_begin(), I->user_end());
#else
    users.assign(I->use_begin(), I->use_end());
#endif
    for (unsigned i=0; i < users.size(); i++) {
        if (T == users[i]) return true;
    }
    return false;
}

bool HybRcvryPass::hasUsersIn(Instruction *I, BasicBlock *B)
{
    assert(NULL != I && NULL != B);
    std::vector<User *> users;
#if LLVM_VERSION >= 40
    users.assign(I->user_begin(), I->user_end());
#else
    users.assign(I->use_begin(), I->use_end());
#endif

   for (unsigned i=0; i < users.size(); i++) {
       if (dyn_cast<Instruction>(users[i])->getParent() == B) {
           return true;
       }
   }
   return false;
}

StoreInst* HybRcvryPass::getStoreInstOfThisLoadInst(LoadInst *LI, BasicBlock *BB)
{
    // Iterate thru all store insts in BB and check
    // whether dest(SI) == src(LI)
    Value *LISrc = LI->getPointerOperand();
    assert(NULL != LISrc);

    for (BasicBlock::iterator it = BB->begin(), et = BB->end(); it != et; it++) {
        Instruction *I = &(*it);
        StoreInst *SI = dyn_cast<StoreInst>(I);
        if (NULL != SI) {
            if (LISrc == SI->getPointerOperand()) {
                return SI;
            }
        }
    }
    return NULL;
}

void HybRcvryPass::getAllInstsThatBBDependsOn(BasicBlock *BB, std::vector<Instruction *> &BBDependsOnList)
{
    // For every instruction until this BB, get Users and do:
    //  - prepare a list of all insts that have users in BB.
    std::set<Instruction*> BBDependsOnSet;
    Function *F = BB->getParent();
    for (inst_iterator II = inst_begin(F), E = inst_end(F); II != E; II++) {
        Instruction *I = &(*II);
        if (I->getParent() == BB) {
            continue;
        }
        addToListIfBBDependsOnInst(I, BB, BBDependsOnList, &BBDependsOnSet);
    }
    addToListAllInterDependentInsts(BBDependsOnList);
    return;
}

void HybRcvryPass::addToListIfBBDependsOnInst(Instruction *I, BasicBlock *targetBB, std::vector<Instruction*> &BBDependsOnList,
                                                std::set<Instruction*> *uniquenessCheckSet=NULL)
{
    assert(NULL != targetBB);
    if (NULL == uniquenessCheckSet) {
        uniquenessCheckSet = new std::set<Instruction*>();
    }
    std::vector<User*> users;
#if LLVM_VERSION >= 40
    users.assign(I->user_begin(), I->user_end());
#else
    users.assign(I->use_begin(), I->use_end());
#endif
    for (unsigned i=0; i < users.size(); i++) {
        Instruction *userInst = dyn_cast<Instruction>(users[i]);
        if (userInst->getParent() == targetBB) {
            if (uniquenessCheckSet->end() == uniquenessCheckSet->find(I)) {
                uniquenessCheckSet->insert(I);
                BBDependsOnList.push_back(I);
            }
            break;
        }
    }
    return;
}

void HybRcvryPass::addToListAllInterDependentInsts(std::vector<Instruction*> &BBDependsOnList)
{
    std::vector<Instruction*> interDependentInsts;
    std::set<BasicBlock*> BBsInvolved;
    std::set<Instruction*> BBDependsOnSet(BBDependsOnList.begin(), BBDependsOnList.end());

    // First, get all the BBs involved.
    for(unsigned i=0; i < BBDependsOnList.size(); i++) {
        if (BBsInvolved.end() == BBsInvolved.find(BBDependsOnList[i]->getParent())) {
            BBsInvolved.insert(BBDependsOnList[i]->getParent());
        }
    }
    // Then, for all insts in those BBs, are any of their users there in BBDependsOnList?
    unsigned size = 0;
    do {
        interDependentInsts.clear();
        size = BBDependsOnSet.size();
        for (std::set<BasicBlock*>::iterator BI = BBsInvolved.begin(), BE = BBsInvolved.end(); BI != BE; BI++) {
            for (BasicBlock::iterator II = (*BI)->begin(), IE = (*BI)->end(); II != IE; II++) {
                std::vector<User*> users;
                Instruction *I = &(*II);
    #if LLVM_VERSION >= 40
                users.assign(I->user_begin(), I->user_end());
    #else
                users.assign(I->use_begin(), I->use_end());
    #endif
                for(unsigned i=0; i < users.size(); i++){
                    if (BBDependsOnSet.end() != BBDependsOnSet.find(dyn_cast<Instruction>(users[i]))) {
                        interDependentInsts.push_back(I);
                        break;
                    }
                }
            }
        }
        BBDependsOnSet.insert(interDependentInsts.begin(), interDependentInsts.end());
    } while(size < BBDependsOnSet.size());

    BBDependsOnList.assign(BBDependsOnSet.begin(), BBDependsOnSet.end());
    return;
}

GetElementPtrInst* HybRcvryPass::copyOverGEPBefore(Instruction *insertBefore, GetElementPtrInst *GEPInst)
{
    std::vector<Value*> indices;
    GetElementPtrInst *newGEP = NULL;
    indices.assign(GEPInst->idx_begin(), GEPInst->idx_end());
    newGEP = PassUtil::createGetElementPtrInstruction(dyn_cast<Value>(GEPInst->getPointerOperand()),
                                             indices, Twine("GEP.clone"), insertBefore);
    // TODO: copy over any other insts that this GEP depends on too.
    if (NULL != newGEP) {
        GetElementPtrInst *depGEP = dyn_cast<GetElementPtrInst>(newGEP->getPointerOperand());
        if (NULL != depGEP) {
            GetElementPtrInst *newDepGep = copyOverGEPBefore(newGEP, depGEP);
            assert(NULL != newDepGep);
            newGEP->replaceUsesOfWith(depGEP, newDepGep); 
        }
    }
    return newGEP;
}

void HybRcvryPass::createCopiesOfLoadDependencies(BasicBlock *BB, std::vector<Instruction*> &LoadsBBDependsOnList)
{
    for (int i = LoadsBBDependsOnList.size() - 1; i >= 0; i--) {
        LoadInst *LI = dyn_cast<LoadInst>(LoadsBBDependsOnList[i]);
        assert(NULL != LI);
        Instruction *I = dyn_cast<Instruction>(LI);
        std::vector<User *> users;
#if LLVM_VERSION >= 40
        users.assign(I->user_begin(), I->user_end());
#else
        users.assign(I->use_begin(), I->use_end());
#endif
        for (unsigned i=0; i < users.size(); i++) {
            if (BB != dyn_cast<Instruction>(users[i])->getParent()) {
                continue;
            }
            LoadInst *newLI = new LoadInst(LI->getType(), LI->getPointerOperand(), Twine("load.clone"), true,
                          dyn_cast<Instruction>(users[i]));
            assert(NULL != newLI);
            users[i]->replaceUsesOfWith(LI, newLI);

            // Add any loads that this load may depend on.
            User *U = dyn_cast<User>(newLI);
            for (unsigned i=0; i < U->getNumOperands(); i++) {
                Value *V = U->getOperand(i);
                LoadInst *VLI = dyn_cast<LoadInst>(V);
                if (NULL != VLI && (VLI->getParent() != newLI->getParent())) {
                    LoadInst *newDepLI = new LoadInst(VLI->getType(), VLI->getPointerOperand(), Twine("load.clone.dep"), true,
                                                      dyn_cast<Instruction>(newLI));
                    assert(NULL != newDepLI);
                    newLI->replaceUsesOfWith(VLI, newDepLI);
                }
            }
 
            GetElementPtrInst *GEPOperand = dyn_cast<GetElementPtrInst>(LI->getPointerOperand());
            if (NULL != GEPOperand) {
                GetElementPtrInst* newGEP = copyOverGEPBefore(newLI, GEPOperand);
                assert(NULL != newGEP);
                newLI->replaceUsesOfWith(GEPOperand, newGEP);
            }
        }
    }

    return;
}

void HybRcvryPass::adjustInterBlockDependencies(BasicBlock *prevBB, BasicBlock *BB,
                                             std::vector<Instruction *> *dependencies,
                                             BasicBlock *destBB, DominatorTree *DT)
{

    replaceUsesWithAlloca(prevBB, BB);
    return;

    std::set<Instruction *> BBDependsOnSet, inturnTheyDependOn;
    std::vector<Instruction *> BBDependsOnList, LoadsBBDependsOnList, GEPsBBDependsOnList;

    getAllInstsThatBBDependsOn(BB, BBDependsOnList);
    if (0 == BBDependsOnList.size()) {
        // Nothing to do
        return;
    }

    // Replace uses of non-LoadInst and non-GEPs with allocas
    replaceUsesWithAlloca(BBDependsOnList);

    for (unsigned i=0; i < BBDependsOnList.size(); i++) {
        Instruction *I = BBDependsOnList[i];
        if (NULL != dyn_cast<LoadInst>(I)) {
            LoadsBBDependsOnList.push_back(I);
        } else if (NULL != dyn_cast<GetElementPtrInst>(I)) {
            GEPsBBDependsOnList.push_back(I);
        }
    }

    // Loads and GEPs need special attention because, these instructions
    // cannot have inter-basic-block dependencies
    createCopiesOfLoadDependencies(BB, LoadsBBDependsOnList);
    return;
}

split_type HybRcvryPass::locateTheSplit(Instruction *splitPoint, PHINode **PHI)
{
    BasicBlock *B = splitPoint->getParent();
    PHINode *phi = NULL;

    for (BasicBlock::iterator bi = B->begin(); (phi = dyn_cast<PHINode>(bi)); bi++) {
        break;
    }
    if (NULL != phi) {
        /* B is the 'phi successor' - the one that contains the phi instruction. */
        if (NULL != PHI) *PHI = phi;
        return HYBRY_SPLIT_IN_PHI_SUCCESSOR;
    }
    /* B is NOT the phi-successor; so doesn't contain the phinode instruction */
    BasicBlock *succ = B->getSingleSuccessor();
    if (NULL != succ) {
        for (BasicBlock::iterator si = succ->begin(); (phi = dyn_cast<PHINode>(si)); si++) {
            break;
        }
        if (NULL != phi) {
            /* This block is one of the phi blocks.
             * We must rather have the gate before the branch that leads here.
             */
            if (NULL != PHI) *PHI = phi;
            DEBUG(errs() << "phi node found: " << phi->getName() << " in function: " << phi->getParent()->getParent()->getName() << "\n");
            return HYBRY_SPLIT_IN_A_PHI;
        }
    }
    if (NULL != PHI) *PHI = NULL;
    return HYBRY_SPLIT_IN_NON_PHI;
}

split_type HybRcvryPass::getPHISafeSplitPoint(BasicBlock **bbToSplit, Instruction **splitPoint)
{
    // Find out what case is this.
    BasicBlock *B = (*splitPoint)->getParent();
    assert(B == (*bbToSplit));
    PHINode *phi;
    split_type splitTy = locateTheSplit(*splitPoint, &phi);
    BasicBlock *singlePredBB = NULL, *predPredBB = NULL;
    BranchInst *brInstToPhi = NULL, *brInstBeforePredBB = NULL;
    Function *F = B->getParent();
    unsigned predCount = 0;
    switch(splitTy) {
        case HYBRY_SPLIT_IN_PHI_SUCCESSOR:
            DEBUG(errs() << "split_type : HYBRY_SPLIT_IN_PHI_SUCCESSOR\n");
            // this is type 1. We choose to split before the phi predecessor.
            singlePredBB = (*(phi->block_begin()))->getSinglePredecessor();
            brInstToPhi = dyn_cast<BranchInst>(singlePredBB->getTerminator());
            assert(NULL != brInstToPhi);

            // Let's make this brInstToPhi, the splitPoint instead.
            // Then, the entry gate gets inserted, before this PHI related fork occurs.
            *splitPoint = PREV_INST(dyn_cast<Instruction>(brInstToPhi));
            *bbToSplit = (*splitPoint)->getParent();
            break;

        case HYBRY_SPLIT_IN_A_PHI:
            // B is one of the 'phi blocks'
            DEBUG(errs() << "split_type : HYBRY_SPLIT_IN_A_PHI\n");
            singlePredBB = B->getSinglePredecessor();
            assert(NULL != singlePredBB);
            if (&(*(F->begin())) == singlePredBB) {
                predPredBB = singlePredBB;
            } else {
                predPredBB = singlePredBB->getSinglePredecessor(); // is this flaky? not always the case?
                for (BasicBlock *pred : predecessors(singlePredBB)) {
                    assert(NULL != pred);
                    predCount++;
                }
                DEBUG(errs() << "Num predecessor of basicblock in function: " << singlePredBB->getParent()->getName()
                      << " is: " << predCount << "\n");
            }
            assert(NULL != predPredBB);
            brInstBeforePredBB = dyn_cast<BranchInst>(predPredBB->getTerminator());

            *splitPoint = PREV_INST(dyn_cast<Instruction>(brInstBeforePredBB));
            *bbToSplit = (*splitPoint)->getParent();
            break;

        case HYBRY_SPLIT_IN_NON_PHI:

        default:
            DEBUG(errs() << "split_type : HYBRY_SPLIT_IN_NON_PHI\n");
            break;
    }
    return splitTy;
}

/* Apache for example, uses a dynamically sized alloca in function apr_poll().
   While this is not a good practice, where the size is determined by the value of
   one of the function's arguments, it also hinders enabling alternative paths of 
   checkpointing variants. Initialization of such dynamic value based alloca in 
   two variants, breaks domination criteria between def and use of these allocas, 
   when we introduce ability to dynamically switch between the two variants.
   Hence, we opt to replace such allocas, with a static but large constant sized alloca.
*/
void HybRcvryPass::useFlexAllocasByRef(Function &F)
{
    std::map<CallInst *, CallInst *> F_libCallsMap, F_CIsMap;
    std::map<Value *, Value *> F_localsMap;    
    getCloneArtifacts(F, F_libCallsMap, F_CIsMap, F_localsMap);

    BasicBlock *EB = &(F.getEntryBlock());
    BranchInst *variantSwitch = dyn_cast<BranchInst>(EB->getTerminator());
    assert(NULL != variantSwitch);
    assert(variantSwitch->isConditional());
    Instruction *condInst = dyn_cast<Instruction>(variantSwitch->getCondition());
    assert(NULL != condInst);

    BasicBlock *TB = variantSwitch->getSuccessor(0);  // bbclone.call.1 block
    BasicBlock *FB = variantSwitch->getSuccessor(1);  // bbclone.call.2 block
    assert(NULL != TB);
    assert(NULL != FB);

    std::vector<User *> users;
    std::vector<Value*> indices;
    indices.push_back(ZERO_CONSTANT_INT(*M));

    DEBUG(errs() << "useFlexAllocasByRef() on function: " << F.getName() << "\n");    
    DEBUG(errs() << "size of localsMap for this function: " << F_localsMap.size() << "\n");
    for (BasicBlock::iterator I = FB->begin(), E = FB->end(); I != E; I++) {
        Instruction *inst = &(*I);
        if (EB == inst->getParent()) {
            continue; // Dont have to worry about the entry block
        }
        AllocaInst *alloca = dyn_cast<AllocaInst>(inst);
        if (NULL == alloca) {
            continue;
        }
        if (false == alloca->isArrayAllocation()) {
           continue;
        }
        Value *arrSize = alloca->getArraySize();
        if (NULL != dyn_cast<Constant>(arrSize)) {
            continue;
        }
        // alloca's array size depends on runtime value

        DEBUG(errs() << "The alloca: " << inst->getName() << "\n");
        assert(0 != F_localsMap.count(dyn_cast<Value>(alloca)));
        AllocaInst *theOtherAlloca = dyn_cast<AllocaInst>((F_localsMap.find(alloca))->second);
        assert (NULL != theOtherAlloca);

#ifndef HYBRY_ALLOCA_CONST_SIZE
#define HYBRY_ALLOCA_CONST_SIZE		10240
#endif
        AllocaInst *replaceWithConstSizeAlloca = new AllocaInst(alloca->getAllocatedType(),
                                                                CONSTANT_INT(*M, HYBRY_ALLOCA_CONST_SIZE),
                                                                "constsize.alloca", variantSwitch);
        assert(NULL != replaceWithConstSizeAlloca);

        SmallVector <AllocaInst*, 2> theAllocas(2);
        theAllocas[0] = alloca; theAllocas[1] = theOtherAlloca;
        DEBUG(errs() << "replacing users of old allocas.\n");
        for (unsigned i = 0 ; i < 2; i++) {
            curr_cloneId = i;
            users.clear();
#if LLVM_VERSION >= 40
            users.assign(theAllocas[i]->user_begin(), theAllocas[i]->user_end());
#else
            users.assign(theAllocas[i]->use_begin(), theAllocas[i]->use_end());
#endif
            for (unsigned j = 0; j < users.size(); j++) {
                users[j]->replaceUsesOfWith(theAllocas[i], replaceWithConstSizeAlloca);
            }
        }
        curr_cloneId = -1;
        
        users.clear();
#if LLVM_VERSION >= 40
        users.assign(theAllocas[1]->user_begin(), theAllocas[1]->user_end());
#else
        users.assign(theAllocas[1]->use_begin(), theAllocas[1]->use_end());
#endif
        assert(0 == users.size());
        theAllocas[1]->eraseFromParent();
        return;
    } // for ends
    return;
}


Instruction* HybRcvryPass::getValueInstInTheOtherClone(AllocaInst *theAlloca)
{
    Function *F = theAlloca->getParent()->getParent();
    std::vector<User*> allocaUsers;
    PassUtil::getUsers(theAlloca, allocaUsers);
    assert(0 != allocaUsers.size());
    Instruction *thisInst = NULL, *thatInst = NULL;
    StoreInst *thisStoreInst = NULL;
    DEBUG(errs() << "gviitoc: F: " << F->getName() << " theAlloca: " << theAlloca->getName()
            << " first allocauser: " << dyn_cast<Instruction>(allocaUsers[0])->getName() << "\n");
    for (int i= allocaUsers.size() - 1; i >= 0 ; i--) {
        DEBUG(
            errs()<< "\t" << i << ": ";
            allocaUsers[i]->print(errs());
            errs() << "\n"
        );
        thisStoreInst = dyn_cast<StoreInst>(allocaUsers[i]);
        if (NULL != thisStoreInst) {
            thisInst = dyn_cast<Instruction>(thisStoreInst); // this is the storeinst probably added by us.
            break;
        }
    }
    assert(NULL != thisStoreInst);
    Value *thisValue = thisStoreInst->getValueOperand();    // this'd be the instruction from the original program.
    assert(NULL != thisValue);
    assert(NULL != dyn_cast<Instruction>(thisValue));

    uint64_t thisAssignedID = PassUtil::getAssignedID(thisValue, HYBPREP_NAMESPACE_FUNC_INSTRS);
    assert(0 != thisAssignedID);

    DEBUG(errs() << "gviitoc: F: " << F->getName() << " assignedID: " << thisAssignedID << "\n");

    for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; I++) {
        Instruction *inst = &(*I);
        if (inst == thisInst) {
            continue;
        }
        uint64_t id = PassUtil::getAssignedID(inst, HYBPREP_NAMESPACE_FUNC_INSTRS);
        if (id == thisAssignedID){
            thatInst = inst;
            break;
        }
    }
    if (NULL != thatInst) {
        DEBUG(
            errs() << "gviitoc: thatInst: ";
            thatInst->print(errs());
            errs() << "\n");
    }
    return thatInst;
}

AllocaInst* HybRcvryPass::getCorrespondingAlloca(AllocaInst *theAlloca, std::vector<AllocaInst*> *fromAllocas)
{
    AllocaInst __attribute__((unused)) *thatAlloca = NULL;
    Instruction *thatInst = getValueInstInTheOtherClone(theAlloca);
    if (NULL == thatInst) {
        return NULL;
    }
    // Get the intersection of usersOf(thatInst) and usersOf(anyOf(fromAllocas))
    std::vector<User*> thatInstUsers;
    PassUtil::getUsers(thatInst, thatInstUsers);

    for (unsigned i=0; i < fromAllocas->size(); i++) {
        std::vector<User*> allocaUsers;
        PassUtil::getUsers((*fromAllocas)[i], allocaUsers);
        for (unsigned j=0; j < allocaUsers.size(); j++) {
            for (unsigned k=0; k < thatInstUsers.size(); k++) {
                if (allocaUsers[j] == thatInstUsers[k]) {   // found!
                    DEBUG(errs() << "getCorrAlloca: alloca: " << theAlloca->getName() << " corr. alloca: " << (*fromAllocas)[i]->getName() << "\n");
                    return (*fromAllocas)[i];
                }
            }
        }
    }
    return NULL;
}

void HybRcvryPass::fixupNewAllocaUsages(Function &F)
{
    std::map<Function*, std::vector<AllocaInst*>* >::iterator MI1, MI2;
    MI1 = clone1NewAllocasMap.find(&F);
    DEBUG(errs() << " fixupNewAllocaUsages() on function: " << F.getName() << "\n");
    if(clone1NewAllocasMap.end() == MI1) {
        DEBUG(errs() << "clone1NewAllocasMap is empty for function: " << F.getName() << "\n");
        return;
    }
    MI2 = clone2NewAllocasMap.find(&F);
    assert(clone2NewAllocasMap.end() != MI2);

    std::vector<AllocaInst*> *pClone1Allocas, *pClone2Allocas;
    pClone1Allocas = MI1->second;
    pClone2Allocas = MI2->second;
    std::vector<AllocaInst*> allocasToRemove;
    for (unsigned i=0; i < pClone2Allocas->size(); i++) {
        AllocaInst *thatAlloca = (*pClone2Allocas)[i];
        AllocaInst *thisAlloca = getCorrespondingAlloca(thatAlloca, pClone1Allocas);
        if (NULL != thisAlloca) {
            DEBUG(errs() << "fnau: replacing users of thatAlloca: " << thatAlloca->getName() << "with thisAlloca: " << thisAlloca->getName() << "\n");
            std::vector<User *> users;
            PassUtil::getUsers(thisAlloca, users);
            for (unsigned j=0; j < users.size(); j++) {
                users[j]->replaceUsesOfWith(thisAlloca, thatAlloca);
            }
            allocasToRemove.push_back(thisAlloca);
        } else {
            // This is an alloca that we added only in clone2 (the undolog one)
            Instruction* thisInst = getValueInstInTheOtherClone(thatAlloca);
            assert (NULL != thisInst);
            // create storeinst to thatAlloca, right after thisInst.
            StoreInst __attribute__((unused)) *storeToThatAlloca = new StoreInst(thisInst, thatAlloca,
                                                                                 "store to thatAlloca",
                                                                                 NEXT_INST(thisInst));
        }
    }
    for (unsigned i=0; i < allocasToRemove.size(); i++) {
        Instruction *I = dyn_cast<Instruction>(allocasToRemove[i]);
        assert(NULL != I);
        I->eraseFromParent();
    }
    return;
}

}
#endif
