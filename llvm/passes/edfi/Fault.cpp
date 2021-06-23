#include <pass.h>

#include "Fault.h"
#include <llvm/Analysis/LoopInfo.h>

/* FaultType ******************************************************************/

void FaultType::addToModule(Module &M){
    ConstantInt* Constant0 = ConstantInt::get(M.getContext(), APInt(32, StringRef("0"), 10));

    Function* edfi_print_stats_old = M.getFunction("edfi_print_stats_old");
    assert(edfi_print_stats_old && "function edfi_print_stats_old() not found");

    Function* edfi_print_stats = M.getFunction("edfi_print_stats");
    assert(edfi_print_stats && "function edfi_print_stats() not found");

    Instruction *endOfFunc = &(*edfi_print_stats->getEntryBlock().getTerminator());

    GlobalVariable* fault_count = new GlobalVariable(/*Module=*/M, 
            /*Type=*/IntegerType::get(M.getContext(), 32),
            /*isConstant=*/false,
            /*Linkage=*/GlobalValue::ExternalLinkage,
            /*Initializer=*/0, // has initializer, specified below
            /*Name=*/Twine("fault_count_", StringRef(this->getName())));
    fault_count->setAlignment(4);
    fault_count->setInitializer(Constant0);

    this->fault_count = fault_count;

    GlobalVariable* fault_name = new GlobalVariable(/*Module=*/M, 
            /*Type=*/ArrayType::get(IntegerType::get(M.getContext(), 8), strlen(this->getName())+1),
            /*isConstant=*/false,
            /*Linkage=*/GlobalValue::ExternalLinkage,
            /*Initializer=*/0, // has initializer, specified below
            /*Name=*/Twine("fault_name_", StringRef(this->getName())));
    fault_name->setAlignment(1);
    fault_name->setInitializer(PassUtil::getStringConstantArray(M, this->getName()));

    std::vector<Value*> fault_name_ptr_indices;
    fault_name_ptr_indices.push_back(Constant0);
    fault_name_ptr_indices.push_back(Constant0);
    Constant* fault_name_ptr = PassUtil::getGetElementPtrConstant(fault_name, fault_name_ptr_indices); 

    LoadInst* load_fault_count = new LoadInst(fault_count, "", false, endOfFunc);
    load_fault_count->setAlignment(4);

    std::vector<Value*> fault_print_stat_params;
    fault_print_stat_params.push_back(fault_name_ptr);
    fault_print_stat_params.push_back(load_fault_count);

    CallInst* fault_print_stat_call = PassUtil::createCallInstruction(edfi_print_stats_old, fault_print_stat_params, "", endOfFunc);
    fault_print_stat_call->setCallingConv(CallingConv::C);
    fault_print_stat_call->setTailCall(false);
    ATTRIBUTE_SET_TY fault_print_stat_call_PAL;
    fault_print_stat_call->setAttributes(fault_print_stat_call_PAL);
}

/* SwapFault ******************************************************************/

prob_opt
SwapFault::prob("fault-prob-swap",
        cl::desc("Fault Injector: binary operand swap fault probability [0..1] "),
        cl::init(0), cl::NotHidden, cl::ValueRequired);

const char *SwapFault::getName(){
    return "swap";
}

bool SwapFault::isApplicable(Instruction *I){
    return dyn_cast<BinaryOperator>(I) != NULL;
}

bool SwapFault::apply(Instruction *I, SmallVectorImpl<Value *> *replacements){
    BinaryOperator *Op = dyn_cast<BinaryOperator>(I);
    /* switch operands of binary instructions */
    /* todo: if(op1.type == op2.type */
    Value *tmp = Op->getOperand(0);
    Op->setOperand(0, Op->getOperand(1));
    Op->setOperand(1, tmp);
    replacements->push_back(I);
    return true;
}

/* SwapFaultV2 ******************************************************************/

prob_opt
SwapFaultV2::prob("fault-prob-swapv2",
        cl::desc("Fault Injector: non-commutative binary operand swap fault probability [0..1] "),
        cl::init(0), cl::NotHidden, cl::ValueRequired);

const char *SwapFaultV2::getName(){
    return "swap";
}

bool SwapFaultV2::isApplicable(Instruction *I){
    return !I->isCommutative() && dyn_cast<BinaryOperator>(I) != NULL;
}

bool SwapFaultV2::apply(Instruction *I, SmallVectorImpl<Value *> *replacements){
    BinaryOperator *Op = dyn_cast<BinaryOperator>(I);
    /* switch operands of binary instructions */
    /* todo: if(op1.type == op2.type */
    Value *tmp = Op->getOperand(0);
    Op->setOperand(0, Op->getOperand(1));
    Op->setOperand(1, tmp);
    replacements->push_back(I);
    return true;
}

/* NoLoadFault ******************************************************************/

prob_opt
NoLoadFault::prob("fault-prob-no-load",
        cl::desc("Fault Injector: load instruction loading '0' fault probability [0..1] "),
        cl::init(0), cl::NotHidden, cl::ValueRequired);

const char *NoLoadFault::getName(){
    return "no-load";
}

bool NoLoadFault::isApplicable(Instruction *I){
    return dyn_cast<LoadInst>(I) && dyn_cast<LoadInst>(I)->getOperand(0)->getType()->getContainedType(0)->isIntegerTy();
}

bool NoLoadFault::apply(Instruction *I, SmallVectorImpl<Value *> *replacements){
    LoadInst *LI = dyn_cast<LoadInst>(I);
    Value *newValue;
    /* load 0 instead of target value. */
    newValue = Constant::getNullValue(LI->getOperand(0)->getType()->getContainedType(0));
    LI->replaceAllUsesWith(newValue);
    LI->eraseFromParent();
    replacements->push_back(newValue);
    return true;
}

/* RndLoadFault ******************************************************************/

prob_opt
RndLoadFault::prob("fault-prob-rnd-load",
        cl::desc("Fault Injector: load instruction loading 'rnd()' fault probability [0..1] "),
        cl::init(0), cl::NotHidden, cl::ValueRequired);

const char *RndLoadFault::getName(){
    return "rnd-load";
}

bool RndLoadFault::isApplicable(Instruction *I){
    return dyn_cast<LoadInst>(I) && dyn_cast<LoadInst>(I)->getOperand(0)->getType()->getContainedType(0)->isIntegerTy() && I->getType()->isIntegerTy(32);
}

bool RndLoadFault::apply(Instruction *I, SmallVectorImpl<Value *> *replacements){
    Function *RandFunc = I->getParent()->getParent()->getParent()->getFunction("edfi_rand");
    assert(RandFunc);
    LoadInst *LI = dyn_cast<LoadInst>(I);
    Value *newValue = CallInst::Create(RandFunc, "", I);
    LI->replaceAllUsesWith(newValue);
    LI->eraseFromParent();
    replacements->push_back(newValue);
    return true;
}

/* NoStoreFault ******************************************************************/

prob_opt
NoStoreFault::prob("fault-prob-no-store",
        cl::desc("Fault Injector: remove store instruction fault probability [0..1] "),
        cl::init(0), cl::NotHidden, cl::ValueRequired);

const char *NoStoreFault::getName(){
    return "no-store";
}

bool NoStoreFault::isApplicable(Instruction *I){
    return dyn_cast<StoreInst>(I);
}

bool NoStoreFault::apply(Instruction *I, SmallVectorImpl<Value *> *replacements){
    StoreInst *SI = dyn_cast<StoreInst>(I);
    /* remove store instruction */
    SI->eraseFromParent();
    return true;
}

/* FlipBranchFault ******************************************************************/

prob_opt
FlipBranchFault::prob("fault-prob-flip-branch",
        cl::desc("Fault Injector: flip branch fault probability [0..1] "),
        cl::init(0), cl::NotHidden, cl::ValueRequired);

const char *FlipBranchFault::getName(){
    return "flip-branch";
}

bool FlipBranchFault::isApplicable(Instruction *I){
    if(BranchInst *BI = dyn_cast<BranchInst>(I)){
        return BI->isConditional();
    }
    return false;
}

bool FlipBranchFault::apply(Instruction *I, SmallVectorImpl<Value *> *replacements){
    BranchInst *BI = (BranchInst *) I;
    BasicBlock *tmp = BI->getSuccessor(0);
    BI->setSuccessor(0, BI->getSuccessor(1));
    BI->setSuccessor(1, tmp);
    return false;
}

/* StuckAtBranchFault ******************************************************************/

prob_opt
StuckAtBranchFault::prob("fault-prob-stuck-at-branch",
        cl::desc("Fault Injector: choose always the same successor branch fault probability [0..1] "),
        cl::init(0), cl::NotHidden, cl::ValueRequired);

const char *StuckAtBranchFault::getName(){
    return "stuck-at-branch";
}

bool StuckAtBranchFault::isApplicable(Instruction *I){
    if(BranchInst *BI = dyn_cast<BranchInst>(I)){
        return BI->isConditional();
    }
    return false;
}

bool StuckAtBranchFault::apply(Instruction *I, SmallVectorImpl<Value *> *replacements){
    BranchInst *BI = (BranchInst *) I;
    int prevailing_succ = rand() % 2;
    BI->getSuccessor(1-prevailing_succ)->removePredecessor(BI->getParent(), true);
    BranchInst::Create(BI->getSuccessor(prevailing_succ), BI->getParent());
    BI->eraseFromParent();
    return false;
}

/* StuckAtLoopFault ******************************************************************/

prob_opt
StuckAtLoopFault::prob("fault-prob-stuck-at-loop",
        cl::desc("Fault Injector: stay stuck in infinite loop [0..1] "),
        cl::init(0), cl::NotHidden, cl::ValueRequired);

const char *StuckAtLoopFault::getName(){
    return "stuck-at-loop";
}

bool StuckAtLoopFault::isApplicable(Instruction *I){
    
    if(BranchInst *BI = dyn_cast<BranchInst>(I)){
        if(BI->isConditional()){
            LoopInfo &LI = getLoopInfo(BI->getParent()->getParent());
            Loop *L = LI.getLoopFor(BI->getParent());
            if(L){
                int i;
                for(i=0;i < 2; i++){
                    if(L->getHeader() == BI->getSuccessor(i)){
                        return true;
                    }
                }
            }
        };
    }
    return false;
}

bool StuckAtLoopFault::apply(Instruction *I, SmallVectorImpl<Value *> *replacements){

    if(BranchInst *BI = dyn_cast<BranchInst>(I)){
        if(BI->isConditional()){
            LoopInfo &LI = getLoopInfo(BI->getParent()->getParent());
            Loop *L = LI.getLoopFor(BI->getParent());
            if(L){
                int i;
                for(i=0;i < 2; i++){
                    if(L->getHeader() == BI->getSuccessor(i)){
                        BI->getSuccessor(1-i)->removePredecessor(BI->getParent(), true);
                        BranchInst::Create(BI->getSuccessor(i), BI->getParent());
                        BI->eraseFromParent();
                        return false;
                    }
                }
            }
        };
    }


    return false;
}

/* FlipBoolFault ******************************************************************/

prob_opt
FlipBoolFault::prob("fault-prob-flip-bool",
        cl::desc("Fault Injector: flip boolean value fault probability [0..1] "),
        cl::init(0), cl::NotHidden, cl::ValueRequired);

const char *FlipBoolFault::getName(){
    return "flip-bool";
}

bool FlipBoolFault::isApplicable(Instruction *I){
    return I->getType()->isIntegerTy(1);
}

bool FlipBoolFault::apply(Instruction *I, SmallVectorImpl<Value *> *replacements){
    /* boolean value, not necessarily used as branching condition */

    ConstantInt* True = ConstantInt::get(I->getParent()->getParent()->getParent()->getContext(), APInt(1, StringRef("-1"), 10));
    BinaryOperator* Negation = BinaryOperator::Create(Instruction::Xor, I, True, "", I->getNextNode());
    I->replaceAllUsesWith(Negation);
    Negation->setOperand(0, I); /* restore the use in the negation instruction */
    //errs() << "< inserted (after next): ";
    //Negation->print(errs());
    //errs() << "\n";
    replacements->push_back(Negation);
    return false;
}

/* CorruptPointerFault ******************************************************************/

prob_opt
CorruptPointerFault::prob("fault-prob-corrupt-pointer",
        cl::desc("Fault Injector: corrupt pointer fault probability [0..1] "),
        cl::init(0), cl::NotHidden, cl::ValueRequired);

const char *CorruptPointerFault::getName(){
    return "corrupt-pointer";
}

bool CorruptPointerFault::isApplicable(Instruction *I){
    return I->getType()->isPointerTy();
}

bool CorruptPointerFault::apply(Instruction *I, SmallVectorImpl<Value *> *replacements){
    Instruction *next = I->getNextNode();
    Module *M = I->getParent()->getParent()->getParent();
    Function *RandFunc = M->getFunction("edfi_rand");
    CallInst* PtrRandFuncCall = CallInst::Create(RandFunc, "", next);
    CastInst* rand64 = new SExtInst(PtrRandFuncCall, IntegerType::get(M->getContext(), 64), "", next);
    CastInst* rnd_ptr = new IntToPtrInst(rand64, I->getType(), "", next);    
    I->replaceAllUsesWith(rnd_ptr);
    I->eraseFromParent();

    replacements->push_back(PtrRandFuncCall);
    replacements->push_back(rand64);
    replacements->push_back(rnd_ptr);
    return true;
}

/* NullPointerFault ******************************************************************/

prob_opt
NullPointerFault::prob("fault-prob-null-pointer",
        cl::desc("Fault Injector: null pointer fault probability [0..1] "),
        cl::init(0), cl::NotHidden, cl::ValueRequired);

const char *NullPointerFault::getName(){
    return "null-pointer";
}

bool NullPointerFault::isApplicable(Instruction *I){
    return I == I->getParent()->getFirstNonPHI();
}

bool NullPointerFault::apply(Instruction *I, SmallVectorImpl<Value *> *replacements){
    BasicBlock *bb = I->getParent();
    LLVMContext &context = bb->getContext();
    IntegerType *type_value = Type::getInt32Ty(context);
    PointerType *type_ptr = type_value->getPointerTo();
    Constant *null_value = Constant::getNullValue(type_value);
    Constant *null_ptr = Constant::getNullValue(type_ptr);
    StoreInst *si = new StoreInst(null_value, null_ptr, true, bb->getFirstNonPHI());

    replacements->push_back(si);
    return true;
}

/* CorruptIndexFault ******************************************************************/

prob_opt
CorruptIndexFault::prob("fault-prob-corrupt-index",
        cl::desc("Fault Injector: corrupt index fault probability [0..1] "),
        cl::init(0), cl::NotHidden, cl::ValueRequired);

const char *CorruptIndexFault::getName(){
    return "corrupt-index";
}

bool CorruptIndexFault::isApplicable(Instruction *I){
    if(I->getType()->isIntegerTy(32)){
        for(Value::use_iterator U = I->use_begin(), UE = I->use_end(); U != UE; U++){
            if (dyn_cast<GetElementPtrInst>(*U)) {
                return true;
            }
        }
    }
    return false;
}

bool CorruptIndexFault::apply(Instruction *I, SmallVectorImpl<Value *> *replacements){
    if(I->getType()->isIntegerTy(32)){
        for(Value::use_iterator U = I->use_begin(), UE = I->use_end(); U != UE; U++){
            if (dyn_cast<GetElementPtrInst>(*U)) {
                Instruction::BinaryOps opcode;
                if(rand() % 2){
                    opcode=Instruction::Add;
                }else{
                    opcode=Instruction::Sub;
                }
                Module *M = I->getParent()->getParent()->getParent();
                ConstantInt* Constant1 = ConstantInt::get(M->getContext(), APInt(32, StringRef("1"), 10));
                BinaryOperator* Change = BinaryOperator::Create(opcode, I, Constant1, "", I->getNextNode());
                I->replaceAllUsesWith(Change);
                Change->setOperand(0, I); /* restore the use in the change instruction */
                replacements->push_back(Change);
                return false;
            }
        }
    }
    assert(0 && "should not be reached");
    return false;
}

/* CorruptIntegerFault ******************************************************************/

prob_opt
CorruptIntegerFault::prob("fault-prob-corrupt-integer",
        cl::desc("Fault Injector: corrupt integer fault probability [0..1] "),
        cl::init(0), cl::NotHidden, cl::ValueRequired);

const char *CorruptIntegerFault::getName(){
    return "corrupt-integer";
}

bool CorruptIntegerFault::isApplicable(Instruction *I){
    return I->getType()->isIntegerTy(32);
}

bool CorruptIntegerFault::apply(Instruction *I, SmallVectorImpl<Value *> *replacements){
    Instruction::BinaryOps opcode;
    if(rand() % 2){
        opcode=Instruction::Add;
    }else{
        opcode=Instruction::Sub;
    }
    Module *M = I->getParent()->getParent()->getParent();
    ConstantInt* Constant1 = ConstantInt::get(M->getContext(), APInt(32, StringRef("1"), 10));
    BinaryOperator* Change = BinaryOperator::Create(opcode, I, Constant1, "", I->getNextNode());
    I->replaceAllUsesWith(Change);
    Change->setOperand(0, I); /* restore the use in the change instruction */
    replacements->push_back(Change);
    return false;
}

/* CorruptOperatorFault ******************************************************************/

prob_opt
CorruptOperatorFault::prob("fault-prob-corrupt-operator",
        cl::desc("Fault Injector: corrupt binary operator fault probability [0..1] "),
        cl::init(0), cl::NotHidden, cl::ValueRequired);

const char *CorruptOperatorFault::getName(){
    return "corrupt-operator";
}

bool CorruptOperatorFault::isApplicable(Instruction *I){
    return dyn_cast<BinaryOperator>(I);
}

bool CorruptOperatorFault::apply(Instruction *I, SmallVectorImpl<Value *> *replacements){
    BinaryOperator *BO = dyn_cast<BinaryOperator>(I);

    Instruction::BinaryOps newOpCode;

    if(BO->getType()->isIntOrIntVectorTy()){
        Instruction::BinaryOps IntOps[] = {BinaryOperator::Add, BinaryOperator::Sub, BinaryOperator::Mul, BinaryOperator::UDiv, BinaryOperator::SDiv, BinaryOperator::URem, BinaryOperator::SRem, BinaryOperator::Shl, BinaryOperator::LShr, BinaryOperator::AShr, BinaryOperator::And, BinaryOperator::Or, BinaryOperator::Xor};
        newOpCode = IntOps[rand() % (sizeof(IntOps)/sizeof(IntOps[0]))];
    }else{
        assert(BO->getType()->isFPOrFPVectorTy());
        // FRem is not included, because it requires fmod from libm to be linked in.
        Instruction::BinaryOps FpOps[] = {BinaryOperator::FAdd, BinaryOperator::FSub, BinaryOperator::FMul, BinaryOperator::FDiv};
        newOpCode = FpOps[rand() % (sizeof(FpOps)/sizeof(FpOps[0]))];
    }

    BinaryOperator* Replacement = BinaryOperator::Create(newOpCode, BO->getOperand(0), BO->getOperand(1), "", I->getNextNode());
    BO->replaceAllUsesWith(Replacement);
    BO->eraseFromParent();

    //errs() << "< replaced with: ";
    //Replacement->print(errs());
    //errs() << "\n";
    replacements->push_back(Replacement);
    return true;
}

/* CorruptOperatorFaultV2 ******************************************************************/

prob_opt
CorruptOperatorFaultV2::prob("fault-prob-corrupt-operator-v2",
        cl::desc("Fault Injector: corrupt binary operator fault probability (v2, safe with bit shifting) [0..1] "),
        cl::init(0), cl::NotHidden, cl::ValueRequired);

const char *CorruptOperatorFaultV2::getName(){
    return "corrupt-operator-v2";
}

bool CorruptOperatorFaultV2::isApplicable(Instruction *I){
    if (!dyn_cast<BinaryOperator>(I)) return false;
    if (I->getType() == Type::getIntNTy(I->getContext(), 80)) return false;
    if (I->getType() == Type::getIntNTy(I->getContext(), 144)) return false;
    return true;
}

static void addOp(std::vector<Instruction::BinaryOps> &ops, Instruction::BinaryOps op, BinaryOperator *orig){
    if (op != orig->getOpcode()){
        ops.push_back(op);
    }
}

bool CorruptOperatorFaultV2::apply(Instruction *I, SmallVectorImpl<Value *> *replacements){
    BinaryOperator *BO = dyn_cast<BinaryOperator>(I);

    Instruction::BinaryOps newOpCode;

    std::vector<Instruction::BinaryOps> ops;
    switch(BO->getOpcode()){
    case BinaryOperator::Add:
    case BinaryOperator::Sub:
    case BinaryOperator::Mul:
    case BinaryOperator::UDiv:
    case BinaryOperator::SDiv:
    case BinaryOperator::URem:
    case BinaryOperator::SRem:
    case BinaryOperator::And:
    case BinaryOperator::Or:
    case BinaryOperator::Xor:
        addOp(ops, BinaryOperator::Add, BO);
        addOp(ops, BinaryOperator::Sub, BO);
        addOp(ops, BinaryOperator::Mul, BO);
        addOp(ops, BinaryOperator::UDiv, BO);
        addOp(ops, BinaryOperator::SDiv, BO);
        addOp(ops, BinaryOperator::URem, BO);
        addOp(ops, BinaryOperator::SRem, BO);
        addOp(ops, BinaryOperator::And, BO);
        addOp(ops, BinaryOperator::Or, BO);
        addOp(ops, BinaryOperator::Xor, BO);
	break;
    case BinaryOperator::Shl:
    case BinaryOperator::LShr:
    case BinaryOperator::AShr:
        addOp(ops, BinaryOperator::Shl, BO);
        addOp(ops, BinaryOperator::LShr, BO);
        addOp(ops, BinaryOperator::AShr, BO);
	break;
    case BinaryOperator::FAdd:
    case BinaryOperator::FSub:
    case BinaryOperator::FMul:
    case BinaryOperator::FDiv:
    case BinaryOperator::FRem:
        addOp(ops, BinaryOperator::FAdd, BO);
        addOp(ops, BinaryOperator::FSub, BO);
        addOp(ops, BinaryOperator::FMul, BO);
        addOp(ops, BinaryOperator::FDiv, BO);
        /* addOp(ops, BinaryOperator::FRem, BO); */
	break;
    default:
        break;
    }
    assert(ops.size() > 0);
    newOpCode = ops[rand() % ops.size()];

    BinaryOperator* Replacement = BinaryOperator::Create(newOpCode, BO->getOperand(0), BO->getOperand(1), "", I->getNextNode());
    BO->replaceAllUsesWith(Replacement);
    BO->eraseFromParent();

    //errs() << "< replaced with: ";
    //Replacement->print(errs());
    //errs() << "\n";
    replacements->push_back(Replacement);
    return true;
}

/* WrapperFaultType ******************************************************************/

Instruction *defaultWrap(Module *M, Instruction *I){
    CallSite CS(I); 
    Function *callee = CS.getCalledFunction();
    std::string wrapper_name = std::string("edfi_") + std::string(callee->getName()) + std::string("_wrapper");
    Function *wrapper = M->getFunction(wrapper_name);
    if(!wrapper){
        errs() << "ERROR. can't find wrapper '" << wrapper_name << "'\n";
        exit(1);
    }
    CS.setCalledFunction(wrapper);
    return CS.getInstruction();
}

/* BufferOverflowFault ******************************************************************/

prob_opt
BufferOverflowFault::prob("fault-prob-buffer-overflow",
        cl::desc("Fault Injector: buffer overflow (extending strcpy, memcpy, memmove, memset size). [0..1] "),
        cl::init(0), cl::NotHidden, cl::ValueRequired);

const char *BufferOverflowFault::getName(){
    return "buffer-overflow";
}

Instruction *wrapMemCpy(Module *M, Instruction *I){
    MemCpyInst *ci = dyn_cast<MemCpyInst>(I);
    std::vector<Value*> Args;
    Args.push_back(ci->getRawDest());
    Args.push_back(ci->getRawSource());
    Args.push_back(ci->getLength());
    bool is64bit = dyn_cast<IntegerType>(ci->getLength()->getType())->getBitWidth() == 64;
    CallInst* replacement = PassUtil::createCallInstruction(M->getFunction(is64bit ? "edfi_memcpy64_wrapper" : "edfi_memcpy_wrapper"), Args, "", I);
    I->eraseFromParent();
    return replacement;
}

Instruction *wrapMemSet(Module *M, Instruction *I){
    MemSetInst *ci = dyn_cast<MemSetInst>(I);
    std::vector<Value*> Args;
    Args.push_back(ci->getRawDest());
    Args.push_back(new SExtInst(ci->getValue(), IntegerType::get(M->getContext(), 32), "", I));
    Args.push_back(ci->getLength());
    bool is64bit = dyn_cast<IntegerType>(ci->getLength()->getType())->getBitWidth() == 64;
    CallInst* replacement = PassUtil::createCallInstruction(M->getFunction(is64bit ? "edfi_memset64_wrapper" : "edfi_memset_wrapper"), Args, "", I);
    I->eraseFromParent();
    return replacement;
}

Instruction *wrapMemMove(Module *M, Instruction *I){
    MemMoveInst *ci = dyn_cast<MemMoveInst>(I);
    std::vector<Value*> Args;
    Args.push_back(ci->getRawDest());
    Args.push_back(ci->getRawSource());
    Args.push_back(ci->getLength());
    bool is64bit = dyn_cast<IntegerType>(ci->getLength()->getType())->getBitWidth() == 64;
    CallInst* replacement = PassUtil::createCallInstruction(M->getFunction(is64bit ? "edfi_memmove64_wrapper" : "edfi_memmove_wrapper"), Args, "", I);
    I->eraseFromParent();
    return replacement;
}

void BufferOverflowFault::init_wrappers(Module *M){
    for (Module::iterator it = M->getFunctionList().begin(); it != M->getFunctionList().end(); ++it) {
#if LLVM_VERSION >= 40
        Function *F = &(*it);
#else
        Function *F = it;
#endif
        switch (F->getIntrinsicID()) {
            case Intrinsic::memcpy:
                init_wrapper(F, wrapMemCpy);
            case Intrinsic::memmove:
                init_wrapper(F, wrapMemMove);
            case Intrinsic::memset:
                init_wrapper(F, wrapMemSet);
            default:
                break;
        }
    }

    init_wrapper(M->getFunction("memcpy"), defaultWrap);
    init_wrapper(M->getFunction("memmove"), defaultWrap);
    init_wrapper(M->getFunction("memset"), defaultWrap);

    init_wrapper(M->getFunction("strcpy"), defaultWrap);
    init_wrapper(M->getFunction("strncpy"), defaultWrap);

#if EDFI_USE_DYN_WRAPPER_FAULT_PROBS
    GlobalVariable* dynOverflowProbVar = M->getNamedGlobal("edfi_dyn_overflow_prob");
    dynOverflowProbVar->setInitializer(ConstantFP::get(M->getContext(), APFloat(BufferOverflowFault::prob)));
    BufferOverflowFault::prob.setValue(1);
#endif
}

/* MemLeakFault ******************************************************************/

prob_opt
MemLeakFault::prob("fault-prob-mem-leak",
        cl::desc("Fault Injector: memory leakage (emulated by skipping free()/munmap() calls). [0..1] "),
        cl::init(0), cl::NotHidden, cl::ValueRequired);

const char *MemLeakFault::getName(){
    return "mem-leak";
}

#ifndef LINUX_KERNEL
void MemLeakFault::init_wrappers(Module *M){
    init_wrapper(M->getFunction("free"), defaultWrap);
    init_wrapper(M->getFunction("munmap"), defaultWrap);
}
#else
void MemLeakFault::init_wrappers(Module *M){
    init_wrapper(M->getFunction("kfree"), defaultWrap);
    init_wrapper(M->getFunction("kmem_cache_free"), defaultWrap);
    init_wrapper(M->getFunction("vfree"), defaultWrap);
    init_wrapper(M->getFunction("free_page"), defaultWrap);
    init_wrapper(M->getFunction("free_pages"), defaultWrap);

}
#endif

/* DanglingPointerFault ******************************************************************/

prob_opt
DanglingPointerFault::prob("fault-prob-dangling-pointer",
        cl::desc("Fault Injector: dangling pointers (decrease the value of malloc()’s size argument). [0..1] "),
        cl::init(0), cl::NotHidden, cl::ValueRequired);

const char *DanglingPointerFault::getName(){
    return "dangling-pointer";
}

#ifndef LINUX_KERNEL
void DanglingPointerFault::init_wrappers(Module *M){
    init_wrapper(M->getFunction("malloc"), defaultWrap);
}
#else
void DanglingPointerFault::init_wrappers(Module *M){
    init_wrapper(M->getFunction("kmalloc"), defaultWrap);
    init_wrapper(M->getFunction("vmalloc"), defaultWrap);
    init_wrapper(M->getFunction("kmem_cache_alloc"), defaultWrap);
    init_wrapper(M->getFunction("get_zeroed_page"), defaultWrap);
    init_wrapper(M->getFunction("__get_free_page"), defaultWrap);
    init_wrapper(M->getFunction("__get_free_pages"), defaultWrap);
}

#endif

