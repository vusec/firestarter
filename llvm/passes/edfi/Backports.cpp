#include <pass.h>

#include "Backports.h"

using namespace llvm;

namespace llvm {

//===----------------------------------------------------------------------===//
// Public static methods
//===----------------------------------------------------------------------===//

/// Find the debug info descriptor corresponding to this global variable.
#if LLVM_VERSION >= 37
DIGlobalVariable *Backports::findDbgGlobalDeclare(GlobalVariable *V) {
#else
Value *Backports::findDbgGlobalDeclare(GlobalVariable *V) {
#endif
   return PassUtil::findDbgGlobalDeclare(V);
}

/// Find the debug info descriptor corresponding to this function.
#if LLVM_VERSION >= 37
DISubprogram *Backports::findDbgSubprogramDeclare(Function *V) {
#else
Value *Backports::findDbgSubprogramDeclare(Function *V) {
#endif
   return PassUtil::findDbgSubprogramDeclare(V);
}

/// Finds the llvm.dbg.declare intrinsic corresponding to this value if any.
/// It looks through pointer casts too.
const DbgDeclareInst *Backports::findDbgDeclare(const Value *V) {
  V = V->stripPointerCasts();

  if (!isa<Instruction>(V) && !isa<Argument>(V))
    return 0;

  const Function *F = NULL;
  if (const Instruction *I = dyn_cast<Instruction>(V))
    F = I->getParent()->getParent();
  else if (const Argument *A = dyn_cast<Argument>(V))
    F = A->getParent();

  for (Function::const_iterator FI = F->begin(), FE = F->end(); FI != FE; ++FI)
    for (BasicBlock::const_iterator BI = (*FI).begin(), BE = (*FI).end();
         BI != BE; ++BI)
      if (const DbgDeclareInst *DDI = dyn_cast<DbgDeclareInst>(BI))
        if (DDI->getAddress() == V)
          return DDI;

  return 0;
}

// llvm.dbg.region.end calls, and any globals they point to if now dead.
bool Backports::StripDebugInfo(Module &M) {

    bool Changed = false;

    // Remove all of the calls to the debugger intrinsics, and remove them from
    // the module.
    if (Function *Declare = M.getFunction("llvm.dbg.declare")) {
        while (!Declare->use_empty()) {
#if LLVM_VERSION >= 37
            CallInst *CI = cast<CallInst>(Declare->user_back());
#else
            CallInst *CI = cast<CallInst>(Declare->use_back());
#endif
            CI->eraseFromParent();
        }   
        Declare->eraseFromParent();
        Changed = true;
    }

    if (Function *DbgVal = M.getFunction("llvm.dbg.value")) {
        while (!DbgVal->use_empty()) {
#if LLVM_VERSION >= 37
            CallInst *CI = cast<CallInst>(DbgVal->user_back());
#else
            CallInst *CI = cast<CallInst>(DbgVal->use_back());
#endif
            CI->eraseFromParent();
        }   
        DbgVal->eraseFromParent();
        Changed = true;
    }

    for (Module::named_metadata_iterator NMI = M.named_metadata_begin(),
            NME = M.named_metadata_end(); NMI != NME;) {
#if LLVM_VERSION >= 37
        NamedMDNode *NMD = &(*NMI);
#else
        NamedMDNode *NMD = NMI;
#endif
        ++NMI;
        if (NMD->getName().startswith("llvm.dbg.")) {
            NMD->eraseFromParent();
            Changed = true;
        }   
    }

    for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI)
        for (Function::iterator FI = MI->begin(), FE = MI->end(); FI != FE; 
                ++FI)
            for (BasicBlock::iterator BI = FI->begin(), BE = FI->end(); BI != BE; 
                    ++BI) {
#if LLVM_VERSION >= 37
                if (!BI->getDebugLoc()) {
#else
                if (!BI->getDebugLoc().isUnknown()) {
#endif
                    Changed = true;
                    BI->setDebugLoc(DebugLoc());
                }
            }   

    return Changed;
}

/// SplitBlockAndInsertIfThen - Split the containing block at the
/// specified instruction - everything before and including Cmp stays
/// in the old basic block, and everything after Cmp is moved to a
/// new block. The two blocks are connected by a conditional branch
/// (with value of Cmp being the condition).
/// Before:
///   Head
///   Cmp
///   Tail
/// After:
///   Head
///   Cmp
///   if (Cmp)
///     ThenBlock
///   Tail
///
/// If Unreachable is true, then ThenBlock ends with
/// UnreachableInst, otherwise it branches to Tail.
/// Returns the NewBasicBlock's terminator.

TerminatorInst *Backports::SplitBlockAndInsertIfThen(Instruction *Cmp) {
    Instruction *SplitBefore = Cmp->getNextNode();
    BasicBlock *Head = SplitBefore->getParent();
    BasicBlock *Tail = Head->splitBasicBlock(SplitBefore);
    TerminatorInst *HeadOldTerm = Head->getTerminator();
    LLVMContext &C = Head->getContext();
    BasicBlock *ThenBlock = BasicBlock::Create(C, "", Head->getParent(), Tail);
    BranchInst::Create(/*ifTrue*/ThenBlock, /*ifFalse*/Tail, Cmp, HeadOldTerm);
    HeadOldTerm->eraseFromParent();
    return BranchInst::Create(Tail, ThenBlock);
}

}
