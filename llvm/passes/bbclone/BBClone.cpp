#include <bbclone/BBClonePass.h>

using namespace llvm;

static cl::list<std::string>
mapOpt("bbclone-map",
    cl::desc("Specify all the comma-separated section_regex/function_regex/clone1_section/clone2_section tuples to define the functions to clone. A NULL clone*_section will assign no named section to the cloned functions."),
    cl::OneOrMore, cl::CommaSeparated, cl::NotHidden, cl::ValueRequired);

static cl::list<std::string>
skipSectionsOpt("bbclone-skip-sections",
    cl::desc("Specify comma separated names of sections to skip bbcloning."),
    cl::ZeroOrMore, cl::CommaSeparated, cl::NotHidden, cl::ValueRequired);

static cl::opt<std::string>
prefixOpt("bbclone-prefix",
    cl::desc("Specify the prefix to use for cloned functions."),
    cl::init("bbclone."), cl::NotHidden, cl::ValueRequired);

static cl::opt<std::string>
cloneFlagOpt("bbclone-flag",
    cl::desc("Specify the clone flag global variable (to switch between clone1 and clone2)."),
    cl::init("bbclone_flag"), cl::NotHidden, cl::ValueRequired);

static cl::opt<std::string>
numberLibCallsOpt("bbclone-number-libcalls",
    cl::desc("Specify the metadata namespace to number all the libcalls with."),
    cl::init(""), cl::NotHidden, cl::ValueRequired);

static cl::opt<int>
cloneFlagValue1Opt("bbclone-flag-value1",
    cl::desc("Specify the clone flag value to execute clone1."),
    cl::init(0), cl::NotHidden, cl::ValueRequired);

static cl::opt<bool>
cloneInline1Opt("bbclone-inline1",
    cl::desc("Force clone1 inlining."),
    cl::init(false), cl::NotHidden, cl::ValueRequired);

static cl::opt<bool>
cloneInline2Opt("bbclone-inline2",
    cl::desc("Force clone2 inlining."),
    cl::init(false), cl::NotHidden, cl::ValueRequired);

static cl::opt<bool>
cloneInlineLoopsOpt("bbclone-inline-loops",
    cl::desc("Force (extracted) loop inlining."),
    cl::init(true), cl::NotHidden, cl::ValueRequired);

static cl::list<std::string>
cloneExcludeCallstacksOpt("bbclone-exclude-callstacks-to",
    cl::desc("Specify all the comma-separated tuples to specify the functions whose callstacks should not be instrumented. Indirect calls are resolved according to the callee-mapper below (direct calls only by default)."),
    cl::ZeroOrMore, cl::CommaSeparated, cl::NotHidden, cl::ValueRequired);

static cl::opt<unsigned>
calleeMapperOpt("bbclone-callee-mapper",
    cl::desc("Specify the callee mapper type (from dsa_common.h)."),
    cl::init(DSAUtil::CM_DIRECTCALL_ANALYSIS), cl::NotHidden, cl::ValueRequired);

static cl::opt<std::string>
hookClone1Opt("bbclone-clone1-hookname",
      cl::desc("Specify the name of the hook to be placed in the clone1"),
      cl::init(""), cl::NotHidden, cl::ValueRequired);

static cl::opt<std::string>
hookClone2Opt("bbclone-clone2-hookname",
      cl::desc("Specify the name of the hook to be placed in the clone2"),
      cl::init(""), cl::NotHidden, cl::ValueRequired);

static cl::opt<std::string>
excludeVariadicFuncsOpt("bbclone-exclude-variadic-funcs",
      cl::desc("Opt to exclude variadic functions from being cloned and specify the exclusion section name."),
      cl::init(""), cl::NotHidden, cl::ValueRequired);

static cl::list<std::string>
excludeFuncsOpt("bbclone-exclude-funcs",
    cl::desc("Specify all the comma-separated function names to exlude and put to exclusion section"),
    cl::ZeroOrMore, cl::CommaSeparated, cl::NotHidden, cl::ValueRequired);

static cl::opt<std::string>
ckptHooksPrefixOpt("bbclone-ckpthooks-prefix",
    cl::desc("Specify prefix of the checkpointing hooks if any, to exclude from instrumentations."),
    cl::init("ltckpt_"), cl::NotHidden, cl::ValueRequired);

static cl::list<std::string>
removeCkptHooksSectionOpt("bbclone-rm-ckpthooks-from-section",
    cl::desc("Specify sections in which checkpointing instrumentation need to be removed."),
    cl::ZeroOrMore, cl::CommaSeparated, cl::NotHidden, cl::ValueRequired);

/*
 * Example usage (ltckpt, strip required when instrumenting loops due to bugs in loop-extract):
 *  ./build.llvm [strip loop-extract] bbclone "bbclone-map=(^\$)|(^[^l].*\$)/^.*$/ltckpt_functions/NULL" bbclone-flag=sa_window__is_open bbclone-exclude-callstacks-to=sef_handle_message [bbclone-inline1=1] [bbclone-inline2=1] [debug-only=bbclone]
 */

STATISTIC(numClonedFunctions, "Number of functions cloned");
STATISTIC(numExcludedVariadicFunctions, "Number of variadic functions excluded from cloning");
STATISTIC(numExcludedSpecifiedFuncs, "Number of specified functions excluded from cloning");
STATISTIC(NumLtckptHooksRemoved,      "Number of checkpointing callinsts removed (from specified section).");

namespace llvm {

PASS_COMMON_INIT_ONCE();
DSA_UTIL_INIT_ONCE();

//===----------------------------------------------------------------------===//
// Constructors, destructor, and operators
//===----------------------------------------------------------------------===//

BBClonePass::BBClonePass() : ModulePass(ID) {}

//===----------------------------------------------------------------------===//
// Public methods
//===----------------------------------------------------------------------===//


void BBClonePass::getAnalysisUsage(AnalysisUsage &AU) const
{
#if LLVM_VERSION < 37
    AU.addRequired<DATA_LAYOUT_TY>();
#endif
    DSAUtil::calleeMapper = (DSAUtil::CalleeMapperTy) (unsigned) calleeMapperOpt;
}

bool BBClonePass::runOnModule(Module &M) {

    if ( 0 != BBClonePass::PassRunCount)
    {
	DEBUG(errs() << "Not running this pass (bbclone) again.");
	return false;
    }

    moduleInit(M);

    DEBUG(errs() << "bbclone-clone1-hookname: " << hookClone1Opt << "\n"); 
    DEBUG(errs() << "bbclone-clone2-hookname: " << hookClone2Opt << "\n"); 
    
    getHooks();

    if ("" != numberLibCallsOpt) {
        numberLibCalls(numberLibCallsOpt);
    }

    /* Clone all the functions we can find. */
    getSkipFunctions();
    cloneFunctions();

    /* Inline loops when requested. */
    if (cloneInlineLoopsOpt)
        inlineLoops();

    BBClonePass::PassRunCount++;

    this->ckptHooksPrefix = ckptHooksPrefixOpt;
    if ("" != this->ckptHooksPrefix) {
        Module::FunctionListType &funcs = M.getFunctionList();
        for (Module::iterator it = funcs.begin(); it != funcs.end(); it++) {
            Function *F = &(*it);
            if (F->isIntrinsic() || F->isDeclaration()) {
                continue;
            }
        
            if (PassUtil::isInAnyOfSections(*F, removeCkptHooksSectionOpt)) {
                removeCkptHooks(*F);
            }
        }
    }
    return cloned;
}

void BBClonePass::moduleInit(Module &M)
{
    this->M = &M;
    this->cloned = false;
    this->dsau.init(this, &M);
    this->flagGV = M.getNamedGlobal(cloneFlagOpt);
    if (!this->flagGV) {
        errs() << "Clone flag global variable " << cloneFlagOpt << " not found!\n";
        exit(1);
    }

    parseAndInitRegexMap(mapOpt, regexList, regexMap);
}

void BBClonePass::getSkipFunctions()
{
    for (unsigned i=0;i<cloneExcludeCallstacksOpt.size();i++) {
        std::string FName = cloneExcludeCallstacksOpt[i];
        Function *F = M->getFunction(FName);
        if (!F) {
            errs() << "Function " << FName << " not found!\n";
            exit(1);
        }
        dsau.getCallStacksFunctions(F, skipFunctions);
    }
    DEBUG(
    for (std::set<const Function*>::iterator it=skipFunctions.begin();it!=skipFunctions.end();it++) {
        const Function *F = *it;
        errs() << " - Skipping function (exclude-callstacks-to): " << F->getName() << "\n";
    }
    );
}

bool BBClonePass::isInSkipSection(Function &F, std::vector<std::string> &sectionsList)
{
    for (unsigned i=0; i < sectionsList.size(); i++){
        if (0 == std::string(F.getSection()).compare(sectionsList[i])) {
            return true;
        }
    }
    return false;
}

void BBClonePass::numberLibCalls(std::string namespaceName)
{
    Module::FunctionListType &functionList = M->getFunctionList();
    std::vector<Value*> libCallInsts;
    uint64_t lastLCId = 0;

    for (Module::iterator it = functionList.begin(); it != functionList.end(); ++it) {
        Function *F = &*it;
        for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; I++) {
            CallInst *CI = dyn_cast<CallInst>(&(*I));
            if (NULL != CI) {
                if (PassUtil::isLibCallInst(CI)) {
                    libCallInsts.push_back(CI);
                }
            }
        }
    }
    lastLCId = PassUtil::assignIDs(*M, &libCallInsts, namespaceName);
    DEBUG(errs() << "Numbered all the libcalls. Last assigned id: " << lastLCId << "\n");
    return;
}

void BBClonePass::cloneFunctions()
{
#if LLVM_VERSION >= 37
    InlineFunctionInfo inlineFunctionInfo = InlineFunctionInfo(NULL);
#else
    DATA_LAYOUT_TY *DL = &getAnalysis<DATA_LAYOUT_TY>();
    InlineFunctionInfo inlineFunctionInfo = InlineFunctionInfo(NULL, DL);
#endif
    Module::FunctionListType &functionList = M->getFunctionList();
    std::vector<Function *> functions;
    std::set<const Function*>::iterator skipFunctionsIt;
    for (Module::iterator it = functionList.begin(); it != functionList.end(); ++it) {
        Function *F = &*it;
	if (isInSkipSection(*F, skipSectionsOpt)) {
            continue;
        }
        skipFunctionsIt = skipFunctions.find(F);
        if (skipFunctionsIt == skipFunctions.end())
            functions.push_back(F);
    }
    std::set<Function*> excludeFuncs;
    for (unsigned i=0;i<excludeFuncsOpt.size();i++) {
        Function *exF = M->getFunction(excludeFuncsOpt[i]);
        assert(NULL != exF);
        if (0 == excludeFuncs.count(exF)) {
            excludeFuncs.insert(exF);
        }
    }

    for (unsigned i=0;i<functions.size();i++) {
        Function *F = functions[i];
        std::string clone1SectionName;
        std::string clone2SectionName;

        if (F->isIntrinsic() || F->isDeclaration() || !isCloneCandidate(F, clone1SectionName, clone2SectionName)) {
            continue;
        }
        if (F->isVarArg()) {
                // TODO: Solve the problem with variadic function cloning. Argument passing needs to be set right.
                // A function call between entering a variadic function and passing on the same arguments in a callInst to its
                // cloned variadic callee ends up in segfault.
            if ("" != excludeVariadicFuncsOpt) {
                DEBUG(errs() << "Not a candidate for cloning: variadic function: " << F->getName() << "\n");
                F->setSection(excludeVariadicFuncsOpt);
                numExcludedVariadicFunctions++;
                continue;
            }
        }
        if (0 != excludeFuncs.count(F)) {
            errs() << "Excluding function: " << F->getName() << "\n";
            F->setSection(excludeVariadicFuncsOpt);
            numExcludedSpecifiedFuncs++;
            continue;
        }

        numClonedFunctions++;
        StringRef prefix1(prefixOpt);
        StringRef prefix2(prefixOpt);
        Function *clone1 = PassUtil::cloneFunction(F, prefix1.str().append("1.").append(F->getName().str()), clone1SectionName);
        Function *clone2 = PassUtil::cloneFunction(F, prefix2.str().append("2.").append(F->getName().str()), clone2SectionName);

	    DEBUG(errs() << "curr func: " << F->getName() << "[ clones created: " << clone1->getName() << ", " << clone2->getName() << " ]\n");

        BasicBlock *entryBlock = &F->getEntryBlock();
        Instruction *I = &*entryBlock->begin();
        BasicBlock *nextBlock = entryBlock->splitBasicBlock(I, "");

        // Record soon-to-be-dead basic blocks
        std::vector<BasicBlock*> deadBBs;
        for (Function::iterator BI = F->getBasicBlockList().begin(), BE = F->getBasicBlockList().end(); BI != BE; ++BI) {
            BasicBlock *BB = &*BI;
            if (BB == entryBlock)
                continue;
            deadBBs.push_back(BB);
        }

        // Create new basic blocks
        Instruction *branchPoint = entryBlock->getTerminator();
        BasicBlock* call1Block = BasicBlock::Create(M->getContext(), "bbclone.call.1", F, 0);
        BasicBlock* call2Block = BasicBlock::Create(M->getContext(), "bbclone.call.2", F, 0);
        if (NULL != hookClone1)
        {
          if (false == placeCallInstToHook(hookClone1, &*call1Block->begin()))
          {
            DEBUG(errs() << "Hooks: Error while planting hookClone1\n");
          }
        }
        if (NULL != hookClone2)
        {
          if (false == placeCallInstToHook(hookClone2, &*call2Block->begin()))
          {
            DEBUG(errs() << "Hooks: Error while planting hookClone2\n");
          }
        }

        //Replace unconditional branch with compare and conditional branch
        LoadInst* flagGVVal = new LoadInst(flagGV, "", false, branchPoint);
        ICmpInst* flagCmp = new ICmpInst(branchPoint, ICmpInst::ICMP_NE, flagGVVal,
                                            CONSTANT_INT(*M, cloneFlagValue1Opt), "bbclone.icmp");
        BranchInst::Create(call2Block, call1Block, flagCmp, branchPoint);
        branchPoint->eraseFromParent();
        BranchInst::Create(nextBlock, call1Block);
        BranchInst::Create(nextBlock, call2Block);

        // Get arguments
        std::vector<Value*> cloneParams;
        for(Function::arg_iterator it = F->arg_begin(), end = F->arg_end(); it!=end; it++) {
            Argument *arg = &*it;
            cloneParams.push_back(arg);
        }

        // Handle clone2
        branchPoint = call1Block->getTerminator();
        CallInst* call1Inst = PassUtil::createCallInstruction(clone1, cloneParams, "", branchPoint);
        if (!F->getReturnType()->isVoidTy())
            ReturnInst::Create(M->getContext(), call1Inst, call1Block);
        else
            ReturnInst::Create(M->getContext(), call1Block);
        branchPoint->eraseFromParent();

        // Handle clone2
        branchPoint = call2Block->getTerminator();
        CallInst* call2Inst = PassUtil::createCallInstruction(clone2, cloneParams, "", branchPoint);
        if (!F->getReturnType()->isVoidTy())
            ReturnInst::Create(M->getContext(), call2Inst, call2Block);
        else
            ReturnInst::Create(M->getContext(), call2Block);
        branchPoint->eraseFromParent();

        if (NULL != hookClone1)
        {
          if (false == placeCallInstToHook(hookClone1, &*(clone1->getEntryBlock().getFirstInsertionPt())))
          {
            DEBUG(errs() << "Hooks: Error while planting hookClone1\n");
          }
        }
        if (NULL != hookClone2)
        {
          if (false == placeCallInstToHook(hookClone2, &*(clone2->getEntryBlock().getFirstInsertionPt())))
          {
            DEBUG(errs() << "Hooks: Error while planting hookClone2\n");
          }
        }
	
        // Remove instructions from dead basic blocks
        for (unsigned i=0;i<deadBBs.size();i++) {
            BasicBlock *BB = deadBBs[i];
            while (PHINode *PN = dyn_cast<PHINode>(BB->begin())) {
                PN->replaceAllUsesWith(Constant::getNullValue(PN->getType()));
                BB->getInstList().pop_front();
            }
            for (succ_iterator SI = succ_begin(BB), E = succ_end(BB); SI != E; ++SI)
                (*SI)->removePredecessor(BB);
            BB->dropAllReferences();
        }

        // Remove dead basic blocks
        //ProfileInfo *PI = getAnalysisIfAvailable<ProfileInfo>();
        for (unsigned i=0;i<deadBBs.size();i++) {
            BasicBlock *BB = deadBBs[i];
            //if (PI) PI->removeBlock(BB);
            BB->eraseFromParent();
        }

        // Inline calls if requested
        if (cloneInline1Opt) {
            InlineFunction(call1Inst, inlineFunctionInfo);
	        // clone1->addFnAttr(Attribute::AlwaysInline);
	    }
        if (cloneInline2Opt)
	    {
            InlineFunction(call2Inst, inlineFunctionInfo);
	        // clone2->addFnAttr(Attribute::AlwaysInline);
	    }
    }
}

void BBClonePass::inlineLoops()
{
#if LLVM_VERSION >= 37
    InlineFunctionInfo inlineFunctionInfo = InlineFunctionInfo(NULL);
#else
    DATA_LAYOUT_TY *DL = &getAnalysis<DATA_LAYOUT_TY>();
    InlineFunctionInfo inlineFunctionInfo = InlineFunctionInfo(NULL, DL);
#endif
    Module::FunctionListType &functionList = M->getFunctionList();
    for (Module::iterator it = functionList.begin(); it != functionList.end(); ++it) {
        Function *F = &*it;
        for (Function::iterator BI = F->getBasicBlockList().begin(), BE = F->getBasicBlockList().end(); BI != BE; ++BI) {
            BasicBlock *BB = &*BI;
            if (BB->getName().compare("codeRepl")) //naming used by CodeExtractor::extractCodeRegion invoked by loop-extract
                continue;
            CallInst *CI = dyn_cast<CallInst>(BB->begin());
            assert(CI && "Bad codeRepl basic block format!");
            Function *IF = CI->getCalledFunction();
            assert(IF && "Bad codeRepl basic block function!");
            InlineFunction(CI, inlineFunctionInfo);
            if (IF->isDefTriviallyDead())
                IF->eraseFromParent();
        }
    }
}

bool BBClonePass::isCloneCandidate(Function *F, std::string &clone1SectionName, std::string &clone2SectionName)
{
    for (std::vector<std::pair<Regex*, Regex*> >::iterator it = regexList.begin(); it != regexList.end(); ++it) {
        std::pair<Regex*, Regex*> regexes = *it;
        regexMapIt = regexMap.find(regexes);
        assert(regexMapIt != regexMap.end());
        if (isCloneCandidateFromRegexes(F, regexes)) {
            clone1SectionName = regexMapIt->second.first;
            clone2SectionName = regexMapIt->second.second;
            return true;
        }
    }

    return false;
}

bool BBClonePass::isCloneCandidateFromRegexes(Function *F, std::pair<Regex*, Regex*> regexes)
{
    Regex* sectionRegex = regexes.first;
    Regex* functionRegex = regexes.second;
    if(!sectionRegex->match(F->getSection(), NULL)
        || !functionRegex->match(F->getName(), NULL)) {
        return false;
    }

    return true;
}

bool BBClonePass::parseStringTwoKeyMapOpt(std::map<std::pair<std::string, std::string>, std::pair<std::string, std::string> > &map, std::vector<std::pair<std::string, std::string> > &keyList, std::vector<std::string> &stringList)
{
    for (std::vector<std::string>::iterator it = stringList.begin(); it != stringList.end(); ++it) {
        StringRef token = *it;
        SmallVector< StringRef, 4 > tokenVector;
        token.split(tokenVector, "/");
        if(tokenVector.size() != 4) {
            return false;
        }
        StringRef value2 = tokenVector.pop_back_val();
        StringRef value1 = tokenVector.pop_back_val();
        StringRef key2 = tokenVector.pop_back_val();
        StringRef key1 = tokenVector.pop_back_val();
        std::pair<std::string, std::string> key = std::pair<std::string, std::string>(key1, key2);
        std::pair<std::string, std::string> value = std::pair<std::string, std::string>(value1, value2);
        map.insert( std::pair<std::pair<std::string, std::string>, std::pair<std::string, std::string> >(key, value) );
        keyList.push_back(key);
    }

    return true;
}

void BBClonePass::parseAndInitRegexMap(cl::list<std::string> &stringListOpt, std::vector<std::pair<Regex*, Regex*> > &regexList, std::map<std::pair<Regex*, Regex*>, std::pair<std::string, std::string> > &regexMap)
{
    std::map<std::pair<std::string, std::string>, std::pair<std::string, std::string> > stringMap;
    std::vector<std::pair<std::string, std::string> > stringList;
    if (!parseStringTwoKeyMapOpt(stringMap, stringList, stringListOpt) || !initRegexMap(regexMap, regexList, stringMap, stringList)) {
        stringListOpt.error("Invalid format!");
        exit(1);
    }
}

bool BBClonePass::initRegexMap(std::map<std::pair<Regex*, Regex*>, std::pair<std::string, std::string> > &regexMap, std::vector<std::pair<Regex*, Regex*> > &regexList, std::map<std::pair<std::string, std::string>, std::pair<std::string, std::string> > &stringMap, std::vector<std::pair<std::string, std::string> > &stringList)
{
    std::map<std::pair<std::string, std::string>, std::pair<std::string, std::string> >::iterator stringMapIt;
    for (std::vector<std::pair<std::string, std::string> >::iterator it = stringList.begin(); it != stringList.end(); ++it) {
        std::pair<std::string, std::string> key = *it;
        std::string sectionKey = key.first;
        std::string functionKey = key.second;
        stringMapIt = stringMap.find(key);
        assert(stringMapIt != stringMap.end());
        std::pair<std::string, std::string> value = stringMapIt->second;
        std::string clone1Section = value.first;
        std::string clone2Section = value.second;
        std::string error;
        Regex *sectionRegex = new Regex(sectionKey, 0);
        if(!sectionRegex->isValid(error)) {
            errs() << "Error: Invalid section regex.\n";
            return false;
        }
        Regex *functionRegex = new Regex(functionKey, 0);
        if(!functionRegex->isValid(error)) {
            errs() << "Error: Invalid function regex.\n";
            return false;
        }
        if (clone1Section.size()==0) {
            errs() << "Error: Invalid clone1 section.\n";
            return false;
        }
        if (clone2Section.size()==0) {
            errs() << "Error: Invalid clone2 section.\n";
            return false;
        }
        std::pair<Regex*, Regex*> regexes = std::pair<Regex*, Regex*>(sectionRegex, functionRegex);
        DEBUG(errs() << "Using regex " << sectionKey << "/" << functionKey << " with clone1 section " << clone1Section << " and clone2 section " << clone2Section << "\n");
        if (!clone1Section.compare("NULL"))
            clone1Section="";
        if (!clone2Section.compare("NULL"))
            clone2Section="";
        regexMap.insert(std::pair<std::pair<Regex*, Regex*>, std::pair<std::string, std::string> >(regexes, std::pair<std::string, std::string>(clone1Section, clone2Section)));
        regexList.push_back(regexes);
    }

    return true;
}

bool BBClonePass::getHooks()
{
    bool retVal = false;
  	DEBUG(errs() << "Getting hooks.\n");

    if ("" != hookClone1Opt)
    {
    	Constant *clone1HookFunc = M->getFunction(hookClone1Opt);
    	assert(clone1HookFunc != NULL);
      hookClone1  = cast<Function>(clone1HookFunc);
    	hookClone1->setCallingConv(CallingConv::Fast);
      retVal = (NULL != hookClone1);
    }
    else
    {
      hookClone1 = NULL;
      retVal = true;
    }

    if ("" != hookClone2Opt)
    {
    	Constant *clone2HookFunc = M->getFunction(hookClone2Opt);
    	assert(clone2HookFunc != NULL);
    	hookClone2  = cast<Function>(clone2HookFunc);
    	hookClone2->setCallingConv(CallingConv::Fast);
      retVal = retVal && (NULL != hookClone2);
    }
    else
    {
      hookClone2 = NULL;
      retVal = retVal && true;
    }

    if (false == retVal)
    {
      DEBUG(errs() << "Hooks: Error locating specified hooks: "
                   << hookClone1Opt << " and " << hookClone2Opt);
    }
    return retVal;
}

bool BBClonePass::placeCallInstToHook(Function* hook, Instruction *nextInst)
{
  if (NULL == hook || NULL == nextInst)
  {
    return false;
  }
  
  DEBUG(errs() << "Placing callInst to hook: " << hook->getName() << "\n");
  
  std::vector<Value*> args;
  CallInst *callInstToHook = PassUtil::createCallInstruction(hook, args, "", nextInst);
  callInstToHook->setCallingConv(CallingConv::Fast);

  return true;
}

void BBClonePass::removeCkptHooks(Function &F)
{
    std::vector<CallInst*> hooksToRemove;
    for (inst_iterator I = inst_begin(&F), E = inst_end(&F); I != E; I++) {
        CallInst *CI = dyn_cast<CallInst>(&(*I));
        if (NULL != CI) {
            Function *calledFunc = CI->getCalledFunction();
            if ((NULL == calledFunc) || (PassUtil::isLibCallInst(CI))) {
                continue;
            }
            if (std::string::npos != calledFunc->getName().find(this->ckptHooksPrefix)) {
                hooksToRemove.push_back(CI);
            }
        }
    }
    NumLtckptHooksRemoved += hooksToRemove.size();

    for (unsigned i=0; i < hooksToRemove.size(); i++) {
        hooksToRemove[i]->eraseFromParent();
    }
    return;
}

} // end namespace

char BBClonePass::ID = 1;
static RegisterPass<BBClonePass> WP("bbclone", "BBClone Pass");
