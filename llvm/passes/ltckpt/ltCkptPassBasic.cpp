#define DEBUG_TYPE "ltckptbasic"
#include "ltckpt/ltCkptPassWC.h"

#include <llvm/Support/CommandLine.h>
#if LLVM_VERSION >= 37
#include <llvm/IR/Dominators.h>
#else
#include <llvm/Analysis/Dominators.h>
#endif
#include <llvm/Analysis/MemoryBuiltins.h>



STATISTIC(NumStores,         "Number of Stores visited");
STATISTIC(NumMemIntr,        "Number of MemIntrinsics visited");
STATISTIC(NumInstStores,     "Number of Stores instrumented");
STATISTIC(NumInstMemIntr,    "Number of MemIntrinsics instrumented");
STATISTIC(NumSkippedAllocaStores,  "Number of Stores to Allocas Skipped");
STATISTIC(NumSkippedAllocaPhiStores,  "Number of Stores to PhiAllocas Skipped");
STATISTIC(NumSkippedAllocaMemIntr, "Number of MemIntrinsics to Allocas Skipped");
STATISTIC(NumGlobalStores,     "Number of Stores to Globals");
STATISTIC(NumLoadStores,     "Number of Stores to LoadInst");
STATISTIC(NumPhiStores,     "Number of Stores to PhiNodes");
STATISTIC(NumCallStores,     "Number of Stores to CallInst");
STATISTIC(NumDoubleStores,     "Number of identified DoubleStores");
STATISTIC(NumDoubleStoresEliminated,     "Number of eliminated DoubleStores");
STATISTIC(NumGlobalMemIntr,    "Number of MemIntrinsics to Globals");
STATISTIC(NumSkippedNonEscapingStores,  "Number of stores to short lived objects");
STATISTIC(numberofStructStoresInstrumented,  "Number of struct stores instrumentated");
STATISTIC(NumStructStoresEliminated,  "Number of struct stores eliminated");


class ObjectStoreDomationMap {
  typedef std::pair<Instruction *, APInt> StoreWithOffTy;
  typedef std::list<StoreWithOffTy> DominationChainTy;
  typedef std::list<DominationChainTy *> DominationChainListTy;
  std::map<Value *, DominationChainListTy *>  *map;
  DominatorTree *DT;

public:

  ObjectStoreDomationMap(DominatorTree *DT) {
    this->DT = DT;
    map = new std::map<Value *, DominationChainListTy *>();
  }

  ~ObjectStoreDomationMap() {
    delete map;
  }

  void addStore(Value *V, Instruction *I, APInt offset)
  {
    DominationChainTy *dc;
    DominationChainListTy *dcl;
    StoreWithOffTy dcentry(I, offset);

    /* do we already have a chain for this value? */
    if (map->count(V)==0) {
      dcl = new DominationChainListTy();
      dc = new DominationChainTy();
      dc->push_back(dcentry);
      dcl->push_back(dc);
      map->insert(std::pair<Value *, DominationChainListTy *>(V, dcl));
      return;
    }
    dcl = (*map)[V];
    for (auto it = dcl->begin(); it != dcl->end(); it++) {
      dc = *it;
      Instruction *dc_leader = dc->begin()->first;
      if (DT->dominates(dc_leader, I)) {
        dc->push_back(dcentry);
        return;
      }
      if (DT->dominates(I, dc_leader)) {
        dc->push_front(dcentry);
        return;
      }
    }
    dc = new DominationChainTy();
    dc->push_back(dcentry);
    dcl->push_back(dc);
  }

  void dump(std::set<Instruction *> *toDelSet, LtCkptPass * p) {
    DominationChainListTy *dcl;
    DominationChainTy *dc;
    for (auto map_it = map->begin(); map_it!=map->end(); map_it++) {
      dcl = map_it->second;
      for (auto it = dcl->begin(); it != dcl->end(); it++) {
        dc = *it;
        if (dc->size()>1) {
            map_it->first->getType()->dump();
            auto dcit = dc->begin();
            APInt max,min;
            Instruction *dominator=dc->begin()->first;
            if (dcit != dc->end())
              max = min= dcit->second;
            for (; dcit!=dc->end(); dcit++) {
              toDelSet->insert(dcit->first);
              min = min.getSExtValue() < dcit->second.getSExtValue() ? min : dcit->second;
              max = max.getSExtValue() > dcit->second.getSExtValue() ? max : dcit->second;
            }
            numberofStructStoresInstrumented++;
            p->instrumentRange(map_it->first, min, max, dominator);
        }
      }
    }
  }
};


cl::opt<std::string> TopOfLoopNames("tol",
                                    cl::desc("Specify Top of the Loop function"),
			                              cl::value_desc("top_of_the_loop"),
				                            cl::Required);

cl::opt<bool> ltckptOptSkipAllocas("ltckpt_skip_alloca",
                                   cl::desc("Do not instrument stores to allocas"),
                                   cl::value_desc("Skip allocas"));

cl::opt<bool> ltckptOptFullBUDS("ltckpt_use_fullBU",
                                   cl::desc("use full BUAnalysis"),
                                   cl::value_desc("BU"));

cl::opt<bool> ltckptOptAggrStrStr("ltckpt_aggregate_struct_stores",
                                   cl::desc("Aggregate Stores to Structs"),
                                   cl::value_desc("ASS"));

cl::opt<std::string> ltckptInstrumentSectionName("ltckpt_instrument_section",
                                    cl::desc("Specify name of a particular section to instrument"),
                                    cl::value_desc("section name"),
                                    cl::init(""), cl::ValueRequired);

/* experimental feature, related to dynamic nooping [UNUSED]*/
cl::opt<bool> ltckptOptNoTOLs("ltckpt_no_tols",
                                   cl::desc("Do not add TOLs"),
                                   cl::value_desc("Skip instrumenting TOLs"));

/* experimental feature, related to dynamic nooping [UNUSED]*/
cl::opt<bool> ltckptOptNoStoreHooks("ltckpt_skip_storehooks",
                                   cl::desc("Do not add store hooks"),
                                   cl::value_desc("Skip instrumenting stores"));

static cl::list<std::string>
skipStoreHooksOnFunctions("ltckpt-skip-storehooks-on-functions",
    cl::desc("Specify comma separated names of sections to skip (in bbclone pass)."),
    cl::ZeroOrMore, cl::CommaSeparated, cl::NotHidden, cl::ValueRequired);


bool LtCkptPassBasic::onFunction(Function &F) {

	std::vector<std::string> tol_functions;

	std::stringstream ss(TopOfLoopNames);
	std::string item;
	if (F.getName().equals(confSetupInitHook->getName())) {
		instrumentConfSetup(F);
	}

	while(std::getline(ss, item, ':')) {
		tol_functions.push_back(item);
	}
	if (isInLtckptSection(&F)) {
		return false;
	}
        if ("" != ltckptInstrumentSectionName) {
		/* Do not instrument if not in the specified section */
		if (!isInSection(&F, ltckptInstrumentSectionName)) {
			return false;
		}
	}

	if (F.getName().startswith("__libc"))
		return false;

	if (F.getName().startswith("dl"))
		return false;

	if (F.getName().equals("hypermem_read_impl") ||
		F.getName().equals("hypermem_write_impl"))
		return false;

        if (false == ltckptOptNoTOLs) {
		for (unsigned int i = 0; i< tol_functions.size(); i++) {
                    if (StringRef(tol_functions[i]).equals("lib_calls")) {
                        instrumentLibCallCkpts(F);
                    } else if (F.getName().equals(tol_functions[i])) {
			instrumentTopOfTheLoop(F);
		    }
		}
        }

	if (ltckptOptNoStoreHooks) {
		return true;
	}

	for(unsigned i=0; i < skipStoreHooksOnFunctions.size(); i++) {
		if (0 == skipStoreHooksOnFunctions[i].compare(F.getName())) {
			return true;
		}
  }

  std::set<Value *> oMap;
	std::set<Instruction*> referencingInstructions;

	for (Function::iterator it = F.begin(); it != F.end(); ++it) {
		BasicBlock *bb = &*it;
		bool instrumentStores = true;
		for (BasicBlock::iterator it2 = bb->begin(); it2 != bb->end(); ++it2) {
			Instruction *inst = &*it2;
			/*
			 * For each store instruction iterate over all GVs and
			 * use AA to check if inst can modify GV
			 */
			CallInst *tolInst = dyn_cast<CallInst>(inst);
			tx_type_t txType;

			// See if it is any of the checkpointing boundaries
			if ( (NULL != tolInst) &&
			     (TX_TYPE_INVALID != (txType = (tx_type_t)PassUtil::getAssignedID(inst, LTCKPT_NAMESPACE_TX_TYPE)))) {

			/* An example of region split in terms of stores-instrumentation
			   func(){ 
				__FUNC_START__			instrumentStores = true  
				__TX_TYPE_LTCKPT		instrumentStores = true
				__TX_TYPE_HYBRID		instrumentStores = true
				__TX_TYPE_TSX		    instrumentStores = false   until the next TX window starts
				__TX_TYPE_LTCKPT		instrumentStores = true
				__TX_TYPE_TSX			  instrumentStores = false   until the next TX window starts
				__FUNC_END__			  continue with prev selection until end of func
			   }
			 */
				DEBUG(errs() << "Called TOL func: " << tolInst->getCalledFunction()->getName() << "\n");
				switch (txType) {
					case TX_TYPE_TSX:
						instrumentStores = false;
						break;

					case TX_TYPE_HYBRID:
					case TX_TYPE_LTCKPT:
					default:
						instrumentStores = true;
				}
			}

			// Add ckpt instrumentation as required
			if (instrumentStores) {
				if (isa<StoreInst>(inst)) {
					++NumStores;
					if (storeToBeInstrumented(inst)) {
						referencingInstructions.insert(inst);
					}
				} /* if... */

				if (isa<MemIntrinsic>(inst)) {
					++NumMemIntr;
					if (memIntrinsicToBeInstrumented(inst)) {

						referencingInstructions.insert(inst);
					}
				}
			}
		} /* end for eachInst */
	} /* end for each BB */


  eliminateDoubleStores(&referencingInstructions, &F);
  /* this functions counts the number of objects we double store to */
  if(ltckptOptAggrStrStr)
    aggregateStores(&referencingInstructions,&oMap, &F);

	for (auto it = referencingInstructions.begin();
			it!=referencingInstructions.end();
			it++) {

		Instruction *inst = *it;

		if (inst) {
			if (isa<MemIntrinsic>(inst)) {
        ++NumInstMemIntr;
				instrumentMemIntrinsic(inst);
			}
			if (isa<StoreInst>(inst)) {
        ++NumInstStores;
        instrumentStore(inst);
			}
		}
	}

	return true;
}





void LtCkptPassBasic::aggregateStores(std::set<Instruction *> *IL, std::set<Value *> *oMap, Function *F)
{
  if (F->isDeclaration())
    return;
#if LLVM_VERSION >= 37
  DominatorTree *DT = &getAnalysis<DominatorTreeWrapperPass>(*F).getDomTree();
#else
  DominatorTree *DT = &getAnalysis<DominatorTree>(*F);
#endif
  ObjectStoreDomationMap map(DT);
  for (std::set<Instruction*>::iterator it = IL->begin(); it!=IL->end(); it++)
  {
    if (!isa<StoreInst>(*it))
      continue;
    StoreInst *S = cast<StoreInst>(*it);
    Value *V = S->getPointerOperand();
    APInt Offset(DL->getPointerSizeInBits(cast<PointerType>(
    V->getType())->getAddressSpace()),0);

    assert(Offset.getBitWidth() == DL->getPointerSizeInBits(cast<PointerType>(
    V->getType())->getAddressSpace()) &&
      "The offset must have exactly as many bits as our pointer.");

    // Even though we don't look through PHI nodes, we could be called on an
    // instruction in an unreachable block, which may be on a cycle.
    SmallPtrSet<Value *, 4> Visited;
    Visited.insert(V);

    do {
      if (GEPOperator *GEP = dyn_cast<GEPOperator>(V)) {
        if (!GEP->isInBounds())
          break;
        APInt GEPOffset(Offset);
        if (!GEP->accumulateConstantOffset(*DL, GEPOffset))
          break;
        Offset = GEPOffset;
        V = GEP->getPointerOperand();
      } else if (Operator::getOpcode(V) == Instruction::BitCast) {
        V = cast<Operator>(V)->getOperand(0);
      } else if (GlobalAlias *GA = dyn_cast<GlobalAlias>(V)) {
        V = GA->getAliasee();
      } else {
          break;
      }
        assert(V->getType()->isPointerTy() && "Unexpected operand type!");
#if LLVM_VERSION >= 37
    } while (Visited.insert(V).second);
#else
    } while (Visited.insert(V));
#endif

    map.addStore(V, S, Offset);
  }
  std::set<Instruction *> toDelSet;
  map.dump(&toDelSet, this);
  for (auto it = toDelSet.begin(); it != toDelSet.end() ; it++) {
    if ((IL->find(const_cast<Instruction *>(*it)))!= IL->end()) {
      NumStructStoresEliminated++;
      IL->erase(IL->find(const_cast<Instruction *>(*it)));
    }
  }
}


static void recusercheck(const StoreInst *S, const Value *V, std::set<const Instruction *> *ds, const DominatorTree *DT)
{
#if LLVM_VERSION >= 40
  std::vector<const User *> Vusers(V->user_begin(), V->user_end());
  const User *ui;
#else
  std::vector<User *> Vusers(V->use_begin(), V->use_end());
  User *ui;
#endif

  for (unsigned i=0; i < Vusers.size(); i++) {
    ui = Vusers[i];
    if (isa<StoreInst>(ui)) {
      const StoreInst *ss = dyn_cast<StoreInst>(ui);
      if (DT->dominates(S, ss)) {
        if (V->stripPointerCasts() == ss->getPointerOperand()->stripPointerCasts()) {
          NumDoubleStores++;
          const Instruction *i = dyn_cast<Instruction>(ui);
          ds->insert(cast<Instruction>(i));
        }
      }
    }
    else if (isa<CastInst>(*ui) || isa<BitCastInst>(*ui) || isa<GlobalAlias>(*ui)){
      recusercheck(S, ui, ds, DT);
    }
  }
}

void LtCkptPassBasic::eliminateDoubleStores(std::set<Instruction *> *IL, Function *F)
{
#if LLVM_HAS_DSA
  if (F->isDeclaration())
    return;
#if LLVM_VERSION >= 37
  DominatorTree *DT = &getAnalysis<DominatorTreeWrapperPass>(*F).getDomTree();
#else
  DominatorTree *DT = &getAnalysis<DominatorTree>(*F);
#endif

  std::set<const Instruction *> DS;

  for (std::set<Instruction*>::iterator it = IL->begin();
    it!=IL->end();
    it++) {
      if (!isa<StoreInst>(*it)) {
        continue;
      }
      const StoreInst *S = cast<StoreInst>(*it);
      const Value *V = S->getPointerOperand();
      V = V->stripPointerCasts(); /* should still be save */

      recusercheck(S, V, &DS, DT);

      // for (auto ui = V->use_begin(); ui!=V->use_end(); ui++) {
      //   if (isa<StoreInst>(*ui)) {
      //     const StoreInst *ss = cast<StoreInst>(*ui);
      //     if (DT->dominates(S, ss)) {
      //       if (V->stripPointerCasts() == ss->getPointerOperand()->stripPointerCasts()) {
      //         NumDoubleStores++;
      //         DS.insert(cast<Instruction>(*ui));
      //       }
      //     }
      //   }
      // }
  }

  for (auto it = DS.begin(); it != DS.end() ; it++) {
    if ((IL->find(const_cast<Instruction *>(*it)))!= IL->end()) {
      NumDoubleStoresEliminated++;
      IL->erase(IL->find(const_cast<Instruction *>(*it)));
    }
  }
#endif
}

void LtCkptPassBasic::getAnalysisUsage(AnalysisUsage &AU) const
{
#if LLVM_HAS_DSA
  if (ltckptOptFullBUDS)
    AU.addRequired<BUDataStructures>();
  else
    AU.addRequired<EquivBUDataStructures>();
#endif
  LtCkptPass::getAnalysisUsage(AU);
#if LLVM_VERSION >= 37
  AU.addPreserved<DominatorTreeWrapperPass>();
#else
  AU.addPreserved<DominatorTree>();
#endif
}

bool LtCkptPassBasic::memIntrinsicToBeInstrumented(Instruction *inst)
{
  MemIntrinsic *MI;
  if (!isa<MemIntrinsic>(inst))
      return true;
  MI = cast<MemIntrinsic>(inst);

  Value *V =MI->getDest();
  bool go_on = true;
  while (go_on) {
    V = V->stripInBoundsOffsets();
    go_on=false;
    if (isa<GetElementPtrInst>(V)) {
      GetElementPtrInst *G = cast<GetElementPtrInst>(V);
      V= G->getPointerOperand();
      go_on=true;
    }
  }

  if (isa<GlobalValue>(V)) {
    NumGlobalMemIntr++;
  }

  if (ltckptOptSkipAllocas) {
    if (isa<AllocaInst>(V)) {
      NumSkippedAllocaMemIntr++;
      return false;
    }
  }

	/* so far we have to instrument every memintrinsic */
	return true;
}


static void getValuesForPhiNode(PHINode *P, std::set<PHINode *> *nodes, std::set<Value *> *values)
{
  if (nodes->find(P)!=nodes->end())
    return;
  nodes->insert(P);
  for (unsigned int i = 0; i< P->getNumIncomingValues();i++) {
    Value *V = P->getIncomingValue(i)->stripInBoundsOffsets();
    if (isa<PHINode>(V))
      getValuesForPhiNode(cast<PHINode>(V), nodes, values);
    else
      values->insert(V);
  }
}

bool LtCkptPassBasic::runOnModule(Module &M)
{
#if LLVM_HAS_DSA
  if (ltckptOptFullBUDS)
    EQDS = &getAnalysis<BUDataStructures>();
  else
    EQDS = &getAnalysis<EquivBUDataStructures>();
#endif
  return LtCkptPass::runOnModule(M);
}

bool LtCkptPassBasic::storeToBeInstrumented(Instruction *inst)
{
  StoreInst *S;
  if (!isa<StoreInst>(inst))
      return true;
  S = cast<StoreInst>(inst);

  Value *V = S->getPointerOperand();

  bool go_on = true;

  while (go_on) {
	  V = V->stripInBoundsOffsets();
	  go_on=false;
	  if (isa<GetElementPtrInst>(V)) {
		  GetElementPtrInst *G = cast<GetElementPtrInst>(V);
		  V= G->getPointerOperand();
		  go_on=true;
	  }
  }

  if (isa<GlobalValue>(V)) {
    NumGlobalStores++;
  }

  if (isa<CallInst>(V)) {
    NumCallStores++;
  }

  if (isa<PHINode>(V)) {
    std::set<Value*> values;
    std::set<PHINode*> nodes;
    getValuesForPhiNode(cast<PHINode>(V), &nodes, &values);
    // printf("BBs: %ld, Vals: %ld\n", nodes.size(), values.size());
    if(nodes.size()==values.size()==1) {
      // std::cout << V->getName().str() << "\n";
    }
    bool all_alloca = true;
    for (std::set<Value*>::iterator it = values.begin(); it != values.end(); it++) {
      all_alloca &= isa<AllocaInst>(*it);
    }
    if (all_alloca) {
      if (ltckptOptSkipAllocas) {
        NumSkippedAllocaStores++;
        NumSkippedAllocaPhiStores++;
        return false;
      }
    }
    NumPhiStores++;
  }
  if (isa<LoadInst>(V)) {
    NumLoadStores++;
  }
  if (ltckptOptSkipAllocas) {
      if (isa<AllocaInst>(V)) {
        NumSkippedAllocaStores++;
        return false;
      }
  }

#if LLVM_HAS_DSA
  if (true) {
    DSGraph *dsg = EQDS->getDSGraph(*inst->getParent()->getParent());
    DSNodeHandle  dsnh = dsg->getNodeForValue(V);
    DSNode *dsn = dsnh.getNode();

    /* if the node is complete and not global it will not survive this
     function call */
    if(    dsn
       && !dsn->isGlobalNode()
       && !dsn->isUnknownNode()
       && !dsn->isExternalNode()
       &&  dsn->isCompleteNode() ) {
      NumSkippedNonEscapingStores++;
      return false;
    }
  }
#endif
  return true;
}

LtCkptPassBasic::LtCkptPassBasic():LtCkptPass() {
}

char LtCkptPassBasic::ID = 0;

RegisterPass<LtCkptPassBasic> Y("ltckptbasic", "Lightweight Checkpointing Pass");
