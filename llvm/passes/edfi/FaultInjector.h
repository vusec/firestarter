#include <pass.h>

#include <llvm/Analysis/LoopInfo.h>
#include "Backports.h"

using namespace llvm;

namespace llvm {

class FaultInjector : public ModulePass {

  private:
      std::pair<unsigned, float> random_BB_FIF;
      std::map<std::string, float> functions_FIF;
      std::map<std::string, float> functioncallers_FIF;
      std::map<std::string, float> modules_FIF;
      std::set<std::pair<std::string, int> > fault_selector;

      void parseCLInputs();
      void parseCLIRandomBBFIF(std::string str);
      void parseCLINameFIF(std::string str, std::map<std::string, float> &value);
      void parseCLIFaultSelectorParser(std::string str,
                                       std::set<std::pair<std::string, int> > &value);
  public:
      static char ID;

      FaultInjector();

      virtual void getAnalysisUsage(AnalysisUsage &AU) const;
      virtual bool runOnModule(Module &M);

      LoopInfo &getLoopInfo(Function *F){
#if LLVM_VERSION >= 40
            return getAnalysis<LoopInfoWrapperPass>(*F).getLoopInfo();
#else
            return FaultInjector::getAnalysis<LoopInfo>(*F);
#endif
      }

};

}

#define FAULTINJECTOR_H
#ifndef FAULT_H
#include "Fault.h"
#endif
