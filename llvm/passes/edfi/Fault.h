#include <pass.h>

#include <edfi/common.h>
#define FAULT_H
#ifndef FAULTINJECTOR_H
#include "FaultInjector.h"
#endif

using namespace llvm;

typedef cl::opt<float> prob_opt;

extern prob_opt prob_default;

class FaultType{
private:
    GlobalVariable *fault_count;
public:
    FaultType(){
        num_injected=0;num_candidates=0;
        num_injected_total=0;num_candidates_total=0;
    }

    unsigned long num_injected, num_candidates;
    unsigned long long num_injected_total, num_candidates_total;
    std::vector<Constant*> num_injected_initializer, num_candidates_initializer;

    virtual bool isApplicable(Instruction *I) = 0;
    virtual bool apply(Instruction *I, SmallVectorImpl<Value *> *replacements) = 0;
    virtual const char *getName() = 0;
    virtual float getProbability() = 0;

    void addToModule(Module &M);

    GlobalVariable *getFaultCount(){
        return fault_count;
    }

    bool isApplicableWithFIF(Instruction *I, float fif){
        float prob = getProbability() * fif ;
        return isApplicable(I) && (((float)rand()) / (((unsigned)RAND_MAX)+1)) < prob;
    }
};

#define FAULT_APPLY_MEMBERS \
    bool isApplicable(Instruction *I); \
    bool apply(Instruction *I, SmallVectorImpl<Value *> *replacements);

#define FAULT_DATA_MEMBERS \
    const char *getName();\
    static prob_opt prob;\
    float getProbability(){\
        if(prob.getNumOccurrences() == 0 && prob_default.getNumOccurrences() > 0){\
            return prob_default;\
        }\
        return prob;\
    }

#define FAULT_MEMBERS FAULT_APPLY_MEMBERS FAULT_DATA_MEMBERS

class SwapFault : public FaultType {
    public: FAULT_MEMBERS
};

class SwapFaultV2 : public FaultType {
    public: FAULT_MEMBERS
};

class NoLoadFault : public FaultType {
    public: FAULT_MEMBERS
};

class RndLoadFault : public FaultType {
    public: FAULT_MEMBERS
};

class NoStoreFault : public FaultType {
    public: FAULT_MEMBERS
};

class FlipBranchFault : public FaultType {
    public: FAULT_MEMBERS
};

class StuckAtBranchFault : public FaultType {
    public: FAULT_MEMBERS
};

class StuckAtLoopFault : public FaultType {
    public: FAULT_MEMBERS
    FaultInjector *FI;

    StuckAtLoopFault(FaultInjector *Pass){
        FI = Pass;
    }

    LoopInfo &getLoopInfo(Function *F){
        return FI->getLoopInfo(F);
    }
};

class FlipBoolFault : public FaultType {
    public: FAULT_MEMBERS
};

class CorruptPointerFault : public FaultType {
    public: FAULT_MEMBERS
};

class NullPointerFault : public FaultType {
    public: FAULT_MEMBERS
};

class CorruptIndexFault : public FaultType {
    public: FAULT_MEMBERS
};

class CorruptIntegerFault : public FaultType {
    public: FAULT_MEMBERS
};

class CorruptOperatorFault : public FaultType {
    public: FAULT_MEMBERS
};

class CorruptOperatorFaultV2 : public FaultType {
    public: FAULT_MEMBERS
};

typedef Instruction *(*WrapperCB)(Module *, Instruction *);
class WrapperFaultType : public FaultType {
    
  public:
    std::map<Function *, WrapperCB> function_wrappers_map;
  
    void init_wrapper(Function *F, WrapperCB WCB){
        if(!F){
            return;
        }
        function_wrappers_map.insert(std::make_pair(F, WCB));
    }


    bool isApplicable(Instruction *I){
        CallSite CS(I); 
        Function *callee;
        if((CS.isCall() || CS.isInvoke()) && (callee = CS.getCalledFunction()) != NULL){
            if(function_wrappers_map[callee]){
                return true;
            }
        }
        return false;
    }

    bool apply(Instruction *I, SmallVectorImpl<Value *> *replacements){
        CallSite CS(I); 
        Function *callee = CS.getCalledFunction();
        WrapperCB wrapper = function_wrappers_map[callee];
        Instruction *replacement = (*wrapper)(I->getParent()->getParent()->getParent(), I);
        replacements->push_back(replacement);
        return true;
    }

};

#define WRAPPER_FAULT_MEMBERS(CLASS_NAME) \
    FAULT_DATA_MEMBERS \
    void init_wrappers(Module *M);\
    CLASS_NAME(Module *M){\
        init_wrappers(M);\
    }

class BufferOverflowFault : public WrapperFaultType {
    public:
    WRAPPER_FAULT_MEMBERS(BufferOverflowFault)
};

class MemLeakFault : public WrapperFaultType {
    public:
    WRAPPER_FAULT_MEMBERS(MemLeakFault)
};

class DanglingPointerFault : public WrapperFaultType {
    public:
    WRAPPER_FAULT_MEMBERS(DanglingPointerFault)
};

