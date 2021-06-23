#include <slicer/SlicerPass.h>

#include "Callgraph/Callgraph.h"
#include "Modifies/Modifies.h"
#include "PointsTo/PointsTo.h"

using namespace llvm;

#define SlicerPassLog(M) DEBUG(dbgs() << "SlicerPass: " << M << "\n")

namespace llvm {

void getCallees(CallInst *CI, const ptr::PointsToSets &PS,
                std::set<Function*> callees) {
  Value *stripped = CI->getCalledValue()->stripPointerCasts();

  if (Function *F = dyn_cast<Function>(stripped)) {
    callees.insert(F);
  } else {
    typedef ptr::PointsToSets::PointsToSet PTSet;
    const PTSet &S = getPointsToSet(stripped, PS);
    for (PTSet::const_iterator I = S.begin(), E = S.end(); I != E; ++I)
      if (const Function *F = dyn_cast<Function>(I->first))
        callees.insert((Function*)F);
  }
}

void handleCall(Function *parent,
                CallInst *CI,
                const ptr::PointsToSets &PS,
                std::map<Instruction*, std::set<Function*> > &calleesMap) {
  if (isInlineAssembly(CI))
    return;

  std::set<Function*> callees;
  getCallees(CI, PS, callees);
  calleesMap.insert(std::pair<Instruction*, std::set<Function*> >(CI, callees));
}

void getCalleesMap(Module &M, ptr::PointsToSets const& PS,
	std::map<Instruction*, std::set<Function*> > &calleesMap) {
  typedef Module::iterator FunctionsIter;
  for (FunctionsIter f = M.begin(); f != M.end(); ++f)
    if (!f->isDeclaration() && !memoryManStuff(&*f))
      for (inst_iterator i = inst_begin(*f); i != inst_end(*f); i++)
        if (CallInst *CI = dyn_cast<CallInst>(&*i))
          handleCall(&*f, CI, PS, calleesMap);
}

SlicerPass::SlicerPass() : ModulePass(ID) {}

void SlicerPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

bool SlicerPass::runOnModule(Module &M) {
  ptr::PointsToSets PS;
  {
    ptr::ProgramStructure P(M);
    computePointsToSets(P, PS);
  }
  getCalleesMap(M, PS, calleesMap);
  
  return false;
}

char SlicerPass::ID = 2;
RegisterPass<SlicerPass> SI("slicer", "Slicer Pass");

}

