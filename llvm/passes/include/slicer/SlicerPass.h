#ifndef SLICER_PASS_H

#define SLICER_PASS_H

#include <pass.h>

using namespace llvm;

namespace llvm {

class SlicerPass : public ModulePass
{
public:
    static char ID;
    typedef std::pair<Value*, int> Field; // Value and offset
    SlicerPass();
    void getAnalysisUsage(AnalysisUsage &AU) const;
    bool runOnModule(Module &M);

    std::map<Instruction*, std::set<Function*> > calleesMap;
    std::map<Field, std::set<Field> > pointsToMap;
};

}

#endif
