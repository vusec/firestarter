#include "ltckpt/ltCkptPass.h"
#include <llvm/Support/CommandLine.h>
#include <common/pass_common.h>

#if LLVM_VERSION >= 37
#define DEBUG_TYPE "ltckpt"
#endif

#define LTCKPT_STATIC_FUNCTIONS_SECTION "ltckpt_functions"
#define LTCKPT_HIDDEN_STR_PREFIX        ".str.ltckpt"

#define LTCKPT_METHOD_DEFAULT           "bitmap"

#define LTCKPT_IS_EXTERN_FUNC(F)	(					\
		(!F->isIntrinsic())  						\
		&& (F->isDeclaration() || (NULL == &((F)->getEntryBlock())))	\
	)

cl::opt<bool> ltckpt_inline("ltckpt_inline",
    cl::desc("Do not inline Storeinstrumentation"),
    cl::value_desc("inline_store_inst"));


cl::opt<bool> ltckpt_opt_vm("ltckpt_vm",
    cl::desc("Special casing for MINIX VM"),
    cl::value_desc("special casing for VM"));

cl::opt<std::string>
ltckptMethod("ltckpt-method",
    cl::desc("Specify the checkpointing method to use."),
    cl::init(LTCKPT_METHOD_DEFAULT), cl::NotHidden, cl::ValueRequired);

cl::opt<std::string>
ltckptSaveRegisters("ltckpt-save-registers",
    cl::desc("Specify the method to save register values: asm_save | setjmp."),
    cl::init("asm_save"), cl::NotHidden, cl::ValueRequired);

cl::opt<bool> ltckpt_window_profiling("ltckpt-window_profiling",
    cl::desc("Specify to enable window profiling. (Ensure ltckpt static lib compatibility)"),
    cl::init(false), cl::value_desc("window profiling"));

cl::opt<bool> ltckpt_enable_stkchklog("ltckpt-enable-stkchklog",
    cl::desc("Specify to enable STKCHKLOG to verify stack restoration."),
    cl::init(false), cl::value_desc("window profiling"), cl::Hidden);

PASS_COMMON_INIT_ONCE();

STATISTIC(NumTOLCallsInserted,  "Number of top-of-the-loop instrumented");
STATISTIC(NumLibCallsInstrumented, "Number of library calls instrumented with checkpointing");

bool LtCkptPass::memIntrinsicToBeInstrumented(Instruction *inst)
{
	return false;
}


bool LtCkptPass::storeToBeInstrumented(Instruction *inst)
{
	return false;
}

void LtCkptPass::createHooks(Module &M)
{
	Constant *confSetupInitFunc = M.getFunction(CONF_SETUP_FUNC_NAME);
	assert(confSetupInitFunc != NULL);
	confSetupInitHook    = cast<Function>(confSetupInitFunc);

	Constant *confSetupFunc = LTCKPT_GET_HOOK(M, ltckptMethod, CONF_SETUP_FUNC_NAME);
	assert(confSetupFunc != NULL);
	confSetupHook    = cast<Function>(confSetupFunc);

#ifdef __MINIX
	Constant *vmLateInitFunc = M.getFunction("ltckpt_undolog_vm_late_init");
	assert(vmLateInitFunc != NULL);
	vmLateInitHook  = cast<Function>(vmLateInitFunc);
#endif
	Constant *storeInstFunc = LTCKPT_GET_HOOK(M, ltckptMethod, STORE_HOOK_NAME);
	assert(storeInstFunc != NULL);
	storeInstHook    = cast<Function>(storeInstFunc);

	Constant *topOfTheLoopFunc  = LTCKPT_GET_HOOK(M, ltckptMethod, TOP_OF_THE_LOOP_FUNC_NAME);
	assert(topOfTheLoopFunc  != NULL);
	topOfTheLoopHook = cast<Function>(topOfTheLoopFunc);

	Constant *memcpyFunc  = LTCKPT_GET_HOOK(M, ltckptMethod, MEMCPY_FUNC_NAME);
	assert(memcpyFunc  != NULL);
	memcpyHook = cast<Function>(memcpyFunc);
	storeInstHook->setCallingConv(CallingConv::Fast);
	memcpyHook->setCallingConv(CallingConv::Fast);

	Constant *endOfWindowFunc = LTCKPT_GET_HOOK(M, ltckptMethod, END_OF_THE_WINDOW_FUNC_NAME);
	assert(endOfWindowFunc != NULL);
	endOfWindowHook = cast<Function>(endOfWindowFunc);
	return;
}

bool LtCkptPass::instructionModifiesVar(Module &M, Instruction *inst, GlobalVariable* var)
{
#if LLVM_VERSION >= 40
    ModRefInfo result = AA->getModRefInfo(inst, var,
            							  DL->getTypeSizeInBits(var->getType()->getElementType()));
    return result == MRI_Mod || result == MRI_ModRef;
#else /* LLVM_VERSION < 40 */
	AliasAnalysis::ModRefResult result = AA->getModRefInfo(inst, var,
			DL->getTypeSizeInBits(var->getType()->getElementType()));
	return result == AliasAnalysis::Mod || result == AliasAnalysis::ModRef;
#endif
}

void LtCkptPass::fetchLastTwoFPs(Instruction *insertPt)
{
	if (!ltckpt_enable_stkchklog) {
		return;
	}
	std::vector<Value*> args(1);
	args[0] = ZERO_CONSTANT_INT(*(this->M));
	CallInst *getFPCallInst = PassUtil::createCallInstruction(
					Intrinsic::getDeclaration(this->M, Intrinsic::frameaddress),
					args, "", insertPt);
	assert(NULL != getFPCallInst);
	BitCastInst *BCInstFPAddr1 = new BitCastInst(getFPCallInst, Type::getInt8PtrTy(M->getContext()),
                                		     "", insertPt);
	args[0] = CONSTANT_INT(*(this->M), 1);
	CallInst *getLastFPCallInst = PassUtil::createCallInstruction(
					Intrinsic::getDeclaration(this->M, Intrinsic::frameaddress),
					args, "", insertPt);
	assert(NULL != getLastFPCallInst);
	BitCastInst *BCInstFPAddr2 = new BitCastInst(getLastFPCallInst, Type::getInt8PtrTy(M->getContext()),
                                		     "", insertPt);

	// TODO: Store them to ltckpt_fp_addr1, ltckpt_fp_addr2
	GlobalVariable *FPAddr1GV = this->M->getGlobalVariable(LTCKPT_FP_ADDR1_GV);
	GlobalVariable *FPAddr2GV = this->M->getGlobalVariable(LTCKPT_FP_ADDR2_GV);
	assert(NULL != FPAddr1GV);
	assert(NULL != FPAddr2GV);
	StoreInst *SIFP1 __attribute__((unused)) 
				= new StoreInst(BCInstFPAddr1, FPAddr1GV, "store FP, ltckpt_fp_addr1", insertPt);
	StoreInst *SIFP2 __attribute__((unused))
			    = new StoreInst(BCInstFPAddr2, FPAddr2GV, "store FPprev, ltckpt_fp_addr2", insertPt);
	return;	
}

void LtCkptPass::instrumentRetAddr(Instruction *insertPt)
{
	std::vector<Value*> noargs, args(1), profArgs(2);
	CallInst *getAddrOfRetAddrCallInst = PassUtil::createCallInstruction(
											Intrinsic::getDeclaration(this->M, Intrinsic::addressofreturnaddress),
											noargs, "", insertPt);
	assert(NULL != getAddrOfRetAddrCallInst);
	CallInst *callInst;
	args[0] = new BitCastInst(getAddrOfRetAddrCallInst, Type::getInt8PtrTy(M->getContext()),
				"", insertPt);

	if (ltckpt_window_profiling) {
		profArgs[0] = args[0];
		profArgs[1] = ZERO_CONSTANT_INT(*(this->M));
		callInst = PassUtil::createCallInstruction(storeInstHook, profArgs,"",insertPt);
	} else {
		callInst = PassUtil::createCallInstruction(storeInstHook, args,"",insertPt);
	}
	callInst->setCallingConv(CallingConv::Fast);
	callInst->setIsNoInline();
	return;
}

void LtCkptPass::instrumentStore(Instruction *inst)
{
	Instruction &in =*inst;
	std::vector<Value*> args(1), profArgs(2);

	/* the signature of the storeinsthook is (ptr) */
	args[0] = new BitCastInst(static_cast<StoreInst&>(in).getPointerOperand(),
			Type::getInt8PtrTy(M->getContext()),
			"", inst);

	CallInst *callInst;
	if (ltckpt_window_profiling) {
		profArgs[0] = args[0];
		profArgs[1] = ZERO_CONSTANT_INT(*(this->M));
		callInst = PassUtil::createCallInstruction(storeInstHook, profArgs,"",inst);
	} else {
		callInst = PassUtil::createCallInstruction(storeInstHook, args,"",inst);
	}
	callInst->setCallingConv(CallingConv::Fast);
	callInst->setIsNoInline();
	if(ltckpt_inline) {
#if LLVM_VERSION >= 37
		InlineFunctionInfo inlineFunctionInfo = InlineFunctionInfo(NULL);
#else
		InlineFunctionInfo inlineFunctionInfo = InlineFunctionInfo(NULL, DL);
#endif
		InlineFunction(callInst, inlineFunctionInfo);
	}
}

void LtCkptPass::instrumentRange(Value *ptr, APInt from, APInt size,  Instruction *inst)
{
  std::string type_str;
  llvm::raw_string_ostream rso(type_str);

  std::vector<Value*> args(2), profArgs(3);
  IRBuilder<> IRB(inst->getParent());
  /* Let's create a uint8ptr to the beginning of the object */
  ptr =  new BitCastInst(ptr, Type::getInt8PtrTy(M->getContext()), "", inst);
  /* now let's add the offset */
#if LLVM_VERSION >= 37
  ptr = GetElementPtrInst::Create(Type::getInt8Ty(M->getContext()), ptr, ConstantInt::get(M->getContext(),from),"",inst);
#else
  ptr = GetElementPtrInst::Create(ptr, ConstantInt::get(M->getContext(),from),"",inst);
#endif

  args[0] = ptr;
  args[1] = ConstantInt::get(M->getContext(),size);

  rso << "source type: ";
  args[0]->getType()->print(rso);
  rso << " size_type: ";
  args[1]->getType()->print(rso);

  /* sometimes we get a i64 for the size */
#if LLVM_VERSION >= 37
  if (DL->getPointerSizeInBits() == 32) {
#else
  if (M->getPointerSize() == Module::Pointer32) {
#endif
    if (static_cast<const IntegerType*>(args[1]->getType())->getBitWidth() != 32)  {
      args[1] = new TruncInst(args[1], Type::getInt32Ty(M->getContext()), "", inst);
      rso << " size_T to: ";
      args[1]->getType()->print(rso);
    }
  } else {
    if (static_cast<const IntegerType*>(args[1]->getType())->getBitWidth() != 64)  {
      args[1] = new TruncInst(args[1], Type::getInt64Ty(M->getContext()), "", inst);
      rso << " size_T to: ";
      args[1]->getType()->print(rso);
    }
  }

  ltckptPassLog(rso.str() << "\n");

  CallInst *callInst;
  if (ltckpt_window_profiling) {
	profArgs[0] = args[0];
	profArgs[1] = args[1];
  	profArgs[2] = ZERO_CONSTANT_INT(*(this->M));
  	callInst = PassUtil::createCallInstruction(memcpyHook,profArgs,"",inst);
  } else {
  	callInst = PassUtil::createCallInstruction(memcpyHook,args,"",inst);
  }
  callInst->setCallingConv(CallingConv::Fast);
  callInst->setIsNoInline();
}

void LtCkptPass::instrumentMemIntrinsic(Instruction *inst)
{
	std::string type_str;
	llvm::raw_string_ostream rso(type_str);

	MemIntrinsic &in = static_cast<MemIntrinsic&>(*inst);

	std::vector<Value*> args(2), profArgs(3);

	args[0] = new BitCastInst(in.getDest(), Type::getInt8PtrTy(M->getContext()),
			"", inst);
	args[1] = in.getLength();

	rso << "source type: ";
	args[0]->getType()->print(rso);
	rso << " size_type: ";
	args[1]->getType()->print(rso);

	/* sometimes we get a i64 for the size */
#if LLVM_VERSION >= 37
	if (DL->getPointerSizeInBits() == 32) {
#else
	if (M->getPointerSize() == Module::Pointer32) {
#endif
		if (static_cast<const IntegerType*>(args[1]->getType())->getBitWidth() != 32)  {
			args[1] = new TruncInst(args[1], Type::getInt32Ty(M->getContext()), "", inst);
			rso << " size_T to: ";
			args[1]->getType()->print(rso);
		}
	} else {
		if (static_cast<const IntegerType*>(args[1]->getType())->getBitWidth() != 64)  {
			args[1] = new TruncInst(args[1], Type::getInt64Ty(M->getContext()), "", inst);
			rso << " size_T to: ";
			args[1]->getType()->print(rso);
		}
	}

	ltckptPassLog(rso.str() << "\n");

	CallInst *callInst;
	if (ltckpt_window_profiling) {
		profArgs[0] = args[0]; profArgs[1] = args[1];
  		profArgs[2] = ZERO_CONSTANT_INT(*(this->M));
		callInst = PassUtil::createCallInstruction(memcpyHook,profArgs,"",inst);
	} else {
		callInst = PassUtil::createCallInstruction(memcpyHook,args,"",inst);
	}
	callInst->setCallingConv(CallingConv::Fast);
	callInst->setIsNoInline();
}

bool LtCkptPass::isInLtckptSection(Function *F) {
        return (!std::string(F->getSection()).compare(LTCKPT_STATIC_FUNCTIONS_SECTION)
				|| !std::string(F->getSection()).compare("rcvry_functions"));
}

bool LtCkptPass::isInSection(Function *F, std::string sectionName) {
	return !std::string(F->getSection()).compare(sectionName);
}

bool LtCkptPass::instrumentTopOfTheLoop(Function &F)
{
	std::vector<Value*> args(0), profArgs(1);
	for (Function::iterator it = F.begin(); it != F.end(); ++it) {
		BasicBlock *bb = &*it;
		for (BasicBlock::iterator it2 = bb->begin(); it2 != bb->end(); ++it2) {
			Instruction *inst = &*it2;
			CallInst *CI;
			if (ltckpt_window_profiling) {
				profArgs[0] = ZERO_CONSTANT_INT(*(this->M));
				CI = PassUtil::createCallInstruction(topOfTheLoopHook,profArgs,"",inst);
			} else {
				CI = PassUtil::createCallInstruction(topOfTheLoopHook,args,"",inst);
			}
			NumTOLCallsInserted++;
			return true;
		}
	}
	return true;
}

void LtCkptPass::createSaveRegsHook(std::vector<Value *> &svRegsArgs, Instruction *I)
{
	std::string saveRegsFuncName = "setjmp";
	Function *funcFuncType = NULL;
	svRegsArgs.clear();

        /* Fetch the pointer to the buffer where it stores registers */
        GlobalVariable *saveRegsBufferPtrGV = this->M->getGlobalVariable(LTCKPT_ASM_SAVE_REGS_BUFFER);
	assert(NULL != saveRegsBufferPtrGV);

	if (0 == ltckptSaveRegisters.compare("setjmp")) {
		/* TODO: Note that this part is not working right now */
		funcFuncType = this->M->getFunction(LTCKPT_SETJMP_SAVE_REGS_DUMMY_FUNC);
		svRegsArgs.push_back(new LoadInst(saveRegsBufferPtrGV, "load *ltckpt_registers", I));
	} else {
		funcFuncType = this->M->getFunction(LTCKPT_ASM_SAVE_REGS_DUMMY_FUNC);
		saveRegsFuncName = LTCKPT_ASM_SAVE_REGS_FUNC_NAME;
		svRegsArgs.push_back(saveRegsBufferPtrGV);
		svRegsArgs.push_back(CONSTANT_INT(*(this->M), 1));
	}
	assert(funcFuncType != NULL);
	FunctionType *ltckptSaveRegsFuncType = funcFuncType->getFunctionType();
	Constant *saveRegsFunc = this->M->getOrInsertFunction(saveRegsFuncName,
							      ltckptSaveRegsFuncType);
	saveRegsHook = cast<Function>(saveRegsFunc);
	assert(NULL != saveRegsHook);

}

bool LtCkptPass::instrumentLibCallCkpts(Function &F)
{
	DEBUG(errs() << "instr. lib calls for func: " << F.getName() << "\n");

	// instrumentRetAddr(F.getEntryBlock().getFirstNonPHI());
	/* Get the assembly function that save registers */
	std::vector<Value*> args(0), profArgs(1), svRegsArgs;

	for (Function::iterator it = F.begin(); it != F.end(); ++it) {
                BasicBlock *bb = &*it;
                for (BasicBlock::iterator it2 = bb->begin(); it2 != bb->end(); ++it2) {
                         Instruction *inst = &*it2;
                         CallInst *libCallInst = dyn_cast<CallInst>(inst);
                         Function *targetFunc = NULL;

                         if (NULL != libCallInst) {
                             targetFunc = libCallInst->getCalledFunction();
                             if ((NULL != targetFunc)
				&& LTCKPT_IS_EXTERN_FUNC(libCallInst->getCalledFunction())) {
					DEBUG(errs() << "\t extern func call: " << libCallInst->getCalledFunction()->getName() << "\n");
                                        /* Skip if this libcall is not used anywhere */
                                        std::vector<User *> users;
			                if (PassUtil::callHasNoUsers(libCallInst, users)) {
			                    DEBUG(errs() << "\t Skipping instrumentation for libcall: " << targetFunc->getName() << "\n");
                                            continue;
                                        }
					tx_type_t  txType = (tx_type_t) PassUtil::getAssignedID(libCallInst, TX_NAMESPACE_TX_TYPE);
			 		CallInst   *tolCallInst = NULL, *saveRegsCallInst = NULL;
					createSaveRegsHook(svRegsArgs, libCallInst);
					switch(txType) {
						case TX_TYPE_TSX:
						case TX_TYPE_HYBRID:
								// The ltckpttsx pass adds call instructions to the hook. No need to do this here.
								break;

						case TX_TYPE_INVALID:
							txType = TX_TYPE_LTCKPT;
							// fall through, to add call instr.

						case TX_TYPE_LTCKPT:
						default:
							Twine *instName = new Twine("call " + topOfTheLoopHook->getName());
							if (0 != ltckptSaveRegisters.compare("none")) {   /* If not "none" is specified */
								saveRegsCallInst = PassUtil::createCallInstruction(saveRegsHook,svRegsArgs,"",NEXT_INST(libCallInst));
							}
        						if (ltckpt_window_profiling) {
						                profArgs[0] = ZERO_CONSTANT_INT(*(this->M));
								tolCallInst = PassUtil::createCallInstruction(topOfTheLoopHook,profArgs,"",NEXT_INST(libCallInst));
								PassUtil::createCallInstruction(endOfWindowHook, profArgs, "", libCallInst);
							} else {
								tolCallInst = PassUtil::createCallInstruction(topOfTheLoopHook,args,"",NEXT_INST(libCallInst));
								PassUtil::createCallInstruction(endOfWindowHook, args, "", libCallInst);
							}
							tolCallInst->setMetadata(LTCKPT_NAMESPACE_TX_TYPE, PassUtil::createMDNodeForConstant(this->M, txType));
							fetchLastTwoFPs(tolCallInst);
							instrumentRetAddr(NEXT_INST(tolCallInst));
							NumLibCallsInstrumented++;
								break;
					}
                             }
			}
                }
       }
       return true;
}

bool LtCkptPass::instrumentConfSetup(Function &F)
{
	std::vector<Value*> args(0);
	BasicBlock *BB = &*F.begin();
	Instruction *I = &*BB->begin();
	CallInst *CI = PassUtil::createCallInstruction(confSetupHook, args, "", I);
	CI->setIsNoInline();
	return true;
}

LtCkptPass::LtCkptPass() : ModulePass(ID) {}


void LtCkptPass::getAnalysisUsage(AnalysisUsage &AU) const
{
#if LLVM_VERSION >= 37
  AU.addRequired<DominatorTreeWrapperPass>();
#else
  AU.addRequired<DominatorTree>();
	AU.addRequired<DATA_LAYOUT_TY>();
#endif

#if LLVM_VERSION >= 40
    AU.addRequired<AAResultsWrapperPass>();
#else
    AU.addRequired<AliasAnalysis>();
#endif
}


bool LtCkptPass::onFunction(Function &F) {
	return false;
}


bool LtCkptPass::runOnModule(Module &M)
{

	bool mod = false;

	this->M = &M;
	this->tolSuffix = ltckptMethod;

#if LLVM_VERSION >= 40
/* DMITRII KUVAISKII: AA is not used anyway, so just ignore
	AA = &getAnalysis<AAResultsWrapperPass>().getAAResults();
*/
#else
	AA = &getAnalysis<AliasAnalysis>();
#endif

#if LLVM_VERSION >= 37
	DL = &M.getDataLayout();
#else
	DL = &getAnalysis<DATA_LAYOUT_TY>();
#endif

#if LLVM_VERSION < 40
	if (!AA || !DL) {
		ltckptPassLog("could not get AnalysisPass\n");
	}
#endif

	Module::GlobalListType &globalList = M.getGlobalList();

	for (Module::global_iterator it = globalList.begin(); it != globalList.end(); ++it) {
		GlobalVariable *GV = &*it;
		/* we do not care about constants */
		if (GV->isConstant()) {
			continue;
		}
		globalVariables.push_back(&*it);
	}

	ltckptPassLog("Number of global variables found in Module: " << globalVariables.size() << "\n");

	createHooks(M);

	Module::FunctionListType &funcs = M.getFunctionList();

	for (Module::iterator mi = funcs.begin(), me = funcs.end(); mi!=me ; ++mi) {
		mod |= onFunction(*mi);
		DEBUG(errs() << "onFunction(" << (*mi).getName() << "\n");
		if (ltckpt_opt_vm && mi->getName().startswith("main")) {
			/* we have to place the vm specific init hook here */
			for (auto fi = mi->begin(); fi != mi->end(); ++fi) {
				for (auto bbi = fi->begin(); bbi!= fi->end(); ++bbi) {
					if (CallInst *CI =  dyn_cast<CallInst>(bbi)) {
						Function *CF = CI->getCalledFunction();
						if(CF && CF->getName().equals("init_vm"))
						{
							std::vector<Value*> args(0);
							std::cout << "PLACING VM_LATEINITHOOK\n";
                            ++bbi;
							PassUtil::createCallInstruction(vmLateInitHook, args, "", &*bbi);
						}
					}
				}
			}
		}
	}

        if (ltckpt_window_profiling) {
		this->lastAssignedProfilingId = initializeWindowProfiling(topOfTheLoopHook, 
							LTCKPT_NAMESPACE_TOL "_" + this->tolSuffix, this->lastAssignedProfilingId + 1, 0);
		errs() << "Window Profiling: Last assigned ID for " << LTCKPT_NAMESPACE_TOL "_" + ltckptMethod << " : " << this->lastAssignedProfilingId << "\n";
	}

	ltckptPassLog("Done.\n");

	return mod;
}

uint64_t LtCkptPass::initializeWindowProfiling(Function *hook, std::string namespaceName, uint64_t startId, unsigned siteIdArgNum)
{
	// Assign ids to all hook users (in their own namespaces)
	// Set site_id argument at each of their callsites.
	assert(NULL != hook);
#if LLVM_VERSION >= 40
        std::vector<User*> hookUsers (hook->user_begin(), hook->user_end());
#else
        std::vector<User*> hookUsers (hook->use_begin(), hook->use_end());
#endif
	std::vector<Value*> hookCallInsts;
	for (unsigned i=0; i < hookUsers.size(); i++) {
		CallInst *CI = dyn_cast<CallInst>(hookUsers[i]);
		if (NULL != CI) {
			hookCallInsts.push_back(CI);
		}
	}
	uint64_t lastAssignedId = PassUtil::assignIDs(*this->M, &hookCallInsts, namespaceName, "_LAST_ID_HOLDER", startId, true);
	assert(0 != lastAssignedId);
	
	for (unsigned i=0; i < hookCallInsts.size(); i++) {
		uint64_t siteId = PassUtil::getAssignedID(hookCallInsts[i], namespaceName);
		dyn_cast<CallInst>(hookCallInsts[i])->setArgOperand(siteIdArgNum, CONSTANT_INT(*(this->M), siteId));
	}
	return lastAssignedId;
}

char LtCkptPass::ID = 0;


RegisterPass<LtCkptPass> W("ltckpt", "Lightweight Checkpointing Pass");
