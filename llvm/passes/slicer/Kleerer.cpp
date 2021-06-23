#include "llvm/IR/Attributes.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#if LLVM_VERSION >= 37
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/InstIterator.h"
#else
#include "llvm/PassManager.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Support/InstIterator.h"
#endif
#include "llvm/IR/Module.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/TypeBuilder.h"
#if LLVM_VERSION >=40
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#else
#include "llvm/Bitcode/ReaderWriter.h"
#endif
#include "llvm/Support/raw_ostream.h"
#if LLVM_VERSION >= 40
#include "llvm/Support/FileSystem.h"
#include "llvm/IR/LegacyPassManager.h"
#endif
#include "Callgraph/Callgraph.h"
#include "PointsTo/PointsTo.h"
#include "Slicing/Prepare.h"

using namespace llvm;

namespace {
  class KleererPass : public ModulePass {
  public:
    static char ID;

    KleererPass() : ModulePass(ID) { }

    virtual bool runOnModule(Module &M);

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesAll();
#if LLVM_VERSION < 37
      AU.addRequired<DataLayout>();
#endif
    }
  };
}

class Kleerer {
public:
#if LLVM_VERSION >= 37
  Kleerer(ModulePass &modPass, Module &M, const DataLayout &TD,
#else
  Kleerer(ModulePass &modPass, Module &M, DataLayout &TD,
#endif
          callgraph::Callgraph &CG) : modPass(modPass),
      M(M), TD(TD), CG(CG), C(M.getContext()), intPtrTy(TD.getIntPtrType(C)),
      done(false) {
    voidPtrType = TypeBuilder<void *, false>::get(C);
    size_tType = TypeBuilder<size_t, false>::get(C);
    intType = TypeBuilder<int, false>::get(C);
    uintType = TypeBuilder<unsigned, false>::get(C);
  }

  bool run();

private:
  ModulePass &modPass;
  Module &M;
#if LLVM_VERSION >= 37
  const DataLayout &TD;
#else
  DataLayout &TD;
#endif
  callgraph::Callgraph &CG;
  LLVMContext &C;
  IntegerType *intPtrTy;
  bool done;
  Function *klee_make_symbolic;

  /* types */
  Type *voidPtrType;
  Type *size_tType;
  Type *intType;
  Type *uintType;

  Value *handlePtrArg(BasicBlock *mainBB, Constant *name, PointerType *PT);
  void prepareArguments(Function &F, BasicBlock *mainBB,
                        std::vector<Value *> &params);
  void writeMain(Function &F);

  Constant *get_assert_fail();

  Instruction *createMalloc(BasicBlock *BB, Type *type, unsigned typeSize,
                            Value *arraySize);
  Instruction *call_klee_make_symbolic(Constant *name, BasicBlock *BB,
                                       Type *type, Value *addr,
                                       Value *arraySize = 0);
  Instruction *mallocSymbolic(BasicBlock *BB, Constant *name, Type *elemTy,
                              unsigned typeSize, Value *arrSize);
  void makeGlobalsSymbolic(Module &M, BasicBlock *BB);
  BasicBlock *checkAiState(Function *mainFun, BasicBlock *BB,
                           const DebugLoc &debugLoc);
  void addGlobals(Module &M);
};

static RegisterPass<KleererPass> X("kleerer", "Prepares a module for Klee");
char KleererPass::ID;

static void check(Value *Func, ArrayRef<Value *> Args) {
  FunctionType *FTy =
    cast<FunctionType>(cast<PointerType>(Func->getType())->getElementType());

  assert((Args.size() == FTy->getNumParams() ||
          (FTy->isVarArg() && Args.size() > FTy->getNumParams())) &&
         "XXCalling a function with bad signature!");

  for (unsigned i = 0; i != Args.size(); ++i) {
    if (!(FTy->getParamType(i) == Args[i]->getType())) {
      errs() << "types:\n  ";
      FTy->getParamType(i)->dump();
      errs() << "\n  ";
      Args[i]->getType()->dump();
      errs() << "\n";
    }
    assert((i >= FTy->getNumParams() ||
            FTy->getParamType(i) == Args[i]->getType()) &&
           "YYCalling a function with a bad signature!");
  }
}

static unsigned getTypeSize(const DataLayout &TD, Type *type) {
  if (type->isFunctionTy()) /* it is not sized, weird */
    return TD.getPointerSize();

  if (!type->isSized())
    return 100; /* FIXME */

  if (StructType *ST = dyn_cast<StructType>(type))
    return TD.getStructLayout(ST)->getSizeInBytes();

  return TD.getTypeAllocSize(type);
}

Instruction *Kleerer::createMalloc(BasicBlock *BB, Type *type,
                                   unsigned typeSize, Value *arraySize) {
  Function *F = NULL;
  return CallInst::CreateMalloc(BB, intPtrTy, type,
                                ConstantInt::get(intPtrTy, typeSize),
                                arraySize, F);
}

static Constant *getGlobalString(LLVMContext &C, Module &M,
                                 const StringRef &str) {
  Constant *strArray = ConstantDataArray::getString(C, str);
  GlobalVariable *strVar =
        new GlobalVariable(M, strArray->getType(), true,
                           GlobalValue::PrivateLinkage, strArray, "");
  strVar->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
  strVar->setAlignment(1);

  std::vector<Value *> params;
  params.push_back(ConstantInt::get(TypeBuilder<types::i<32>, true>::get(C), 0));
  params.push_back(ConstantInt::get(TypeBuilder<types::i<32>, true>::get(C), 0));

  return ConstantExpr::getInBoundsGetElementPtr(strArray->getType(), strVar, params);
}

Instruction *Kleerer::call_klee_make_symbolic(Constant *name, BasicBlock *BB,
                                              Type *type, Value *addr,
                                              Value *arraySize) {
  std::vector<Value *> p;

  if (addr->getType() != voidPtrType)
    addr = new BitCastInst(addr, voidPtrType, "", BB);
  p.push_back(addr);

  unsigned typeSize = getTypeSize(TD, type);
  Value *size;

  if (arraySize && typeSize == 1)
    size = arraySize;
  else {
    size = ConstantInt::get(size_tType, typeSize);
    if (arraySize)
      size = BinaryOperator::CreateMul(arraySize, size,
                                     "make_symbolic_size", BB);
  }

  p.push_back(size);
  p.push_back(name);

  check(klee_make_symbolic, p);

  return CallInst::Create(klee_make_symbolic, p);
}

/*
 * it also initializes __ai_state_*
 */
void Kleerer::makeGlobalsSymbolic(Module &M, BasicBlock *BB) {
  Constant *zero = ConstantInt::get(intType, 0);
  for (Module::global_iterator I = M.global_begin(), E = M.global_end();
      I != E; ++I) {
    GlobalVariable &GV = *I;
    if (GV.isConstant() || !GV.hasName())
	continue;
    StringRef GVName = GV.getName();
    if (GVName.startswith("llvm."))
	    continue;
    if (GVName.startswith("__ai_") && !GVName.startswith("__ai_state_"))
	    continue;
/*    errs() << "TU " << GVName << " ";
    GV.getType()->getElementType()->dump();
    errs() << "\n\t" << GVName << "\n";*/
    Constant *glob_str = getGlobalString(C, M, GVName);
    BB->getInstList().push_back(call_klee_make_symbolic(glob_str, BB,
		GV.getType()->getElementType(), &GV));
    if (GVName.startswith("__ai_state_"))
	new StoreInst(zero, &GV, "", true, BB);
  }
}

Constant *Kleerer::get_assert_fail()
{
  Type *constCharPtrTy = TypeBuilder<const char *, false>::get(C);
  AttributeSet attrs = AttributeSet().addAttribute(C,
		  AttributeSet::FunctionIndex, Attribute::NoReturn);
  return M.getOrInsertFunction("__assert_fail", attrs, Type::getVoidTy(C),
                               constCharPtrTy, constCharPtrTy, uintType,
                               constCharPtrTy, NULL);
}

BasicBlock *Kleerer::checkAiState(Function *mainFun, BasicBlock *BB,
                                  const DebugLoc &debugLoc) {
  Module *M = mainFun->getParent();
  Constant *zero = ConstantInt::get(intType, 0);

  BasicBlock *finalBB = BasicBlock::Create(C, "final", mainFun);
  BasicBlock *assBB = BasicBlock::Create(C, "assertBB", mainFun);
  std::vector<Value *> params;
  params.push_back(getGlobalString(C, *M, "leaving function with lock held"));
  params.push_back(getGlobalString(C, *M, "n/a"));
  params.push_back(zero);
  params.push_back(getGlobalString(C, *M, "main"));
  CallInst::Create(get_assert_fail(), params, "", assBB)->setDebugLoc(debugLoc);
  new UnreachableInst(C, assBB);
  Value *sum = zero;

  for (Module::global_iterator I = M->global_begin(), E = M->global_end();
      I != E; ++I) {
    GlobalVariable &ai_state = *I;
    if (!ai_state.hasName() || !ai_state.getName().startswith("__ai_state_"))
      continue;
    Value *ai_stateVal = new LoadInst(&ai_state, "", true, BB);
    sum = BinaryOperator::Create(BinaryOperator::Add, ai_stateVal, sum, "", BB);
  }

  Value *ai_stateIsZero = new ICmpInst(*BB, CmpInst::ICMP_EQ, sum, zero);
  BranchInst::Create(finalBB, assBB, ai_stateIsZero, BB);

  return finalBB;
}

void Kleerer::addGlobals(Module &mainMod) {
  for (Module::global_iterator I = M.global_begin(), E = M.global_end();
       I != E; ++I) {
    GlobalVariable &G = *I;
    if (!G.isDeclaration() || G.hasInitializer())
      continue;
    Constant *xxx = Constant::getNullValue(G.getType()->getElementType());
    G.setInitializer(xxx);
  }
}

struct st_desc {
  unsigned long flag;
#define STF_ONE  1
};

static const struct st_desc *getStDesc(const Type *elemTy) {
  typedef std::map<std::string, struct st_desc> StructMap;

  static const StructMap::value_type structMapData[] = {
    StructMap::value_type("pci_dev", (struct st_desc){ STF_ONE }),
  };

  #define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

  static const StructMap structMap(structMapData,
                                   structMapData + ARRAY_SIZE(structMapData));

  if (const StructType *ST = dyn_cast<StructType>(elemTy))
    if (!ST->isLiteral() && ST->hasName()) {

      /* each struct name has "struct." prepended */
      StructMap::const_iterator I = structMap.find(ST->getName().substr(7));
      if (I != structMap.end())
        return &I->second;
    }

  return NULL;
}

Instruction *Kleerer::mallocSymbolic(BasicBlock *BB, Constant *name,
                                     Type *elemTy, unsigned typeSize,
                                     Value *arrSize) {
  BasicBlock::InstListType &insList = BB->getInstList();
  Instruction *ins = createMalloc(BB, elemTy, typeSize, arrSize);
  insList.push_back(ins);
  insList.push_back(call_klee_make_symbolic(name, BB, elemTy, ins, arrSize));
  return ins;
}

Value *Kleerer::handlePtrArg(BasicBlock *mainBB, Constant *name,
                             PointerType *PT) {
  BasicBlock::InstListType &insList = mainBB->getInstList();
  Instruction *ins;
  Type *elemTy = PT->getElementType();
  const struct st_desc *st_desc = getStDesc(elemTy);
  unsigned typeSize = getTypeSize(TD, elemTy);
  Value *arrSize = NULL;
  if (!st_desc || !(st_desc->flag & STF_ONE)) {
    unsigned count = (1 << 20) / typeSize;
    if (count > 4096)
      count = 4096;

    arrSize = ConstantInt::get(size_tType, count);
  }

  ins = mallocSymbolic(mainBB, name, elemTy, typeSize, arrSize);
  if (arrSize) {
    bool cast = ins->getType() != voidPtrType;
    if (cast)
      insList.push_back(ins = new BitCastInst(ins, voidPtrType));
    ins = GetElementPtrInst::CreateInBounds(ins,
           ConstantInt::get(TypeBuilder<types::i<64>, true>::get(C), 2048));
    insList.push_back(ins);
    if (cast)
      insList.push_back(ins = new BitCastInst(ins, PT));
  }

  return ins;
}

void Kleerer::prepareArguments(Function &F, BasicBlock *mainBB,
                               std::vector<Value *> &params) {
  BasicBlock::InstListType &insList = mainBB->getInstList();

  for (Function::const_arg_iterator I = F.arg_begin(), E = F.arg_end(); I != E;
       ++I) {
    const Value &param = *I;
    Type *type = param.getType();
#ifdef DEBUG_WRITE_MAIN
    errs() << "param\n  ";
    param.print(errs());
    errs() << "\n  type=";
    type->print(errs());
    errs() << "\n";
#endif
    Value *val = NULL;
    Constant *name = getGlobalString(C, M, param.hasName() ? param.getName() :
                                     "noname");
    if (PointerType *PT = dyn_cast<PointerType>(type)) {
      val = handlePtrArg(mainBB, name, PT);
    } else if (IntegerType *IT = dyn_cast<IntegerType>(type)) {
      Instruction *ins;
      insList.push_front(ins = new AllocaInst(IT));
      insList.push_back(call_klee_make_symbolic(name, mainBB, type, ins));
      insList.push_back(ins = new LoadInst(ins));
      val = ins;
    }
    if (val)
      params.push_back(val);
  }
}

void Kleerer::writeMain(Function &F) {
  std::string name = M.getModuleIdentifier() + ".main." + F.getName().str() + ".o";
  Function *mainFun = Function::Create(TypeBuilder<int(), false>::get(C),
                    GlobalValue::ExternalLinkage, "main", &M);
  BasicBlock *mainBB = BasicBlock::Create(C, "entry", mainFun);

  FunctionType *klee_make_symbolicTy =
      TypeBuilder<void(void *, size_t, const char *), false>::get(C);
  klee_make_symbolic = dyn_cast<Function>(
      M.getOrInsertFunction("klee_make_symbolic", klee_make_symbolicTy));
  /* if there was one, it should have the same type, i.e. we got Function */
  assert(klee_make_symbolic);
/*  Function *klee_int = Function::Create(
              TypeBuilder<int(const char *), false>::get(C),
              GlobalValue::ExternalLinkage, "klee_int", &M);*/

//  F.dump();

  std::vector<Value *> params;
  prepareArguments(F, mainBB, params);
//  mainFun->viewCFG();

  makeGlobalsSymbolic(M, mainBB);
  addGlobals(M);
#ifdef DEBUG_WRITE_MAIN
  errs() << "==============\n";
  errs() << mainMod;
  errs() << "==============\n";
#endif
  check(&F, params);

  CallInst::Create(&F, params, "", mainBB);
  BasicBlock *final = checkAiState(mainFun, mainBB, F.back().back().getDebugLoc());
  ReturnInst::Create(C, ConstantInt::get(mainFun->getReturnType(), 0),
                     final);

#ifdef DEBUG_WRITE_MAIN
  mainFun->viewCFG();
#endif

#if LLVM_VERSION >= 40
  std::error_code ErrorInfo;
  raw_fd_ostream out(name.c_str(), ErrorInfo, sys::fs::F_Append);
#else
  std::string ErrorInfo;
  raw_fd_ostream out(name.c_str(), ErrorInfo);
#endif

#if LLVM_VERSION >= 40
  if (!ErrorInfo) {
#else
  if (!ErrorInfo.empty()) {
#endif
    errs() << __func__ << ": cannot write '" << name << "'!\n";
    return;
  }

//  errs() << mainMod;

#if LLVM_VERSION >= 40
  llvm::legacy::PassManager Passes;
#else
  PassManager Passes;
#endif
  Passes.add(createVerifierPass());
  Passes.run(M);

  WriteBitcodeToFile(&M, out);
  errs() << __func__ << ": written: '" << name << "'\n";
  mainFun->eraseFromParent();
//  done = true;
}

bool Kleerer::run() {
  Function *F__assert_fail = M.getFunction("__assert_fail");
  if (!F__assert_fail) /* nothing to find here bro */
    return false;

  callgraph::Callgraph::range_iterator RI = CG.callees(F__assert_fail);
  if (std::distance(RI.first, RI.second) == 0)
    return false;

  const ConstantArray *initFuns = getInitFuns(M);
  assert(initFuns && "No initial functions found. Did you run -prepare?");

  for (ConstantArray::const_op_iterator I = initFuns->op_begin(),
       E = initFuns->op_end(); I != E; ++I) {
    const ConstantExpr *CE = cast<ConstantExpr>(&*I);
    assert(CE->getOpcode() == Instruction::BitCast);
    Function &F = *cast<Function>(CE->getOperand(0));

    callgraph::Callgraph::const_iterator II, EE;
    std::tie(II, EE) = CG.calls(&F);
    for (; II != EE; ++II) {
      const Function *callee = (*II).second;
      if (callee == F__assert_fail) {
        writeMain(F);
        break;
      }
    }
    if (done)
      break;
  }
  return false;
}

bool KleererPass::runOnModule(Module &M) {
#if LLVM_VERSION >= 37
  const DataLayout &TD = M.getDataLayout();
#else
  DataLayout &TD = getAnalysis<DataLayout>();
#endif
  ptr::PointsToSets PS;
  {
    ptr::ProgramStructure P(M);
    computePointsToSets(P, PS);
  }

  callgraph::Callgraph CG(M, PS);

  Kleerer K(*this, M, TD, CG);
  return K.run();
}
