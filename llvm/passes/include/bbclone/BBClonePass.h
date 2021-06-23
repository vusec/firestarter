#ifndef BBCLONE_PASS_H

#define BBCLONE_PASS_H

#define DEBUG_TYPE "bbclone"
#include <pass.h>

#include <common/dsa_common.h>

#define BBCLONE_CLONE1_HOOK      "inc_counter_inwindow"
#define BBCLONE_CLONE2_HOOK      "inc_counter_outsidewindow"

using namespace llvm;

namespace llvm {

class BBClonePass : public ModulePass {

  public:
      static char ID;

      BBClonePass();

      virtual void getAnalysisUsage(AnalysisUsage &AU) const;
      virtual bool runOnModule(Module &M);

  private:
      Module *M;
      GlobalVariable *flagGV;
      DSAUtil dsau;
      std::set<const Function*> skipFunctions;
      bool cloned;
      static unsigned PassRunCount;

      std::map<std::pair<Regex*, Regex*>, std::pair<std::string, std::string> > regexMap;
      std::map<std::pair<Regex*, Regex*>, std::pair<std::string, std::string> >::iterator regexMapIt;
      std::vector<std::pair<Regex*, Regex*> > regexList;
      std::string ckptHooksPrefix;

      Function *hookClone1, *hookClone2;

      void moduleInit(Module &M);
      void getSkipFunctions();
      bool isInSkipSection(Function &F, std::vector<std::string> &sectionsList);
      void numberLibCalls(std::string namespaceName);
      void cloneFunctions();
      void inlineLoops();
      bool isCloneCandidate(Function *F, std::string &clone1SectionName, std::string &clone2SectionName);
      bool isCloneCandidateFromRegexes(Function *F, std::pair<Regex*, Regex*> regexes);
      bool parseStringTwoKeyMapOpt(std::map<std::pair<std::string, std::string>, std::pair<std::string, std::string> > &map, std::vector<std::pair<std::string, std::string> > &keyList, std::vector<std::string> &stringList);
      void parseAndInitRegexMap(cl::list<std::string> &stringListOpt, std::vector<std::pair<Regex*, Regex*> > &regexList, std::map<std::pair<Regex*, Regex*>, std::pair<std::string, std::string> > &regexMap);
      bool initRegexMap(std::map<std::pair<Regex*, Regex*>, std::pair<std::string, std::string> > &regexMap, std::vector<std::pair<Regex*, Regex*> > &regexList, std::map<std::pair<std::string, std::string>, std::pair<std::string, std::string> > &stringMap, std::vector<std::pair<std::string, std::string> > &stringList);
      bool getHooks();
      bool placeCallInstToHook(Function* hook, Instruction *nextInst);
      void removeCkptHooks(Function &F);
};

unsigned BBClonePass::PassRunCount = 0;
}

#endif
