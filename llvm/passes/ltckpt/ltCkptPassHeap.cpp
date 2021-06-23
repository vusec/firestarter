#include "ltckpt/ltCkptPassHeap.h"


STATISTIC(DoublesRemoved,         "Number of doubles stores instrument eleminations");


bool LtCkptPassHeap::memIntrinsicToBeInstrumented(Instruction *inst)
{
	return true;
}

#if 1
bool LtCkptPassHeap::doubleCheckStore(StoreInst *inst)
{
	BasicBlock& BB = *inst->getParent();
	Function& F = *BB.getParent();
	AliasAnalysis::Location loc = AA->getLocation(inst);
	Value * v1 = inst->getPointerOperand();
	Value * v2;


	DT   = &getAnalysis<DominatorTree>(F);

	for (Function::iterator FI = F.begin(), FE = F.end(); FI != FE; ) {
		BasicBlock& _B = *FI++;

		/* Do we have to go through this basic block to reach BB?*/
		if(!DT->dominates(&_B,&BB)) {
			continue;
		}

		for (BasicBlock::iterator _BI = _B.begin(), _BE = _B.end(); _BI != _BE; ) {
			Instruction* i = _BI++;
			/*
			 * It is possible that we are in the original basic block.
			 * For this one we only care to what happens to be before the
			 * store instruction we are currently trying to eliminate instrumentation
			 * for */
			if (inst==i) {
				break;
			}

			if (isa<StoreInst>(*i)) {
				StoreInst *si = static_cast<StoreInst*>(i);
				v2 = si->getPointerOperand();
				AliasAnalysis::Location loc2 = AA->getLocation(si);

				if(loc.Ptr == loc2.Ptr) {
					return false;
				}

				if (v1==v2) {
					return false;
				}

			} /* isa<Store...  */

		} /* for (BasicBlock ... */

	} /* for(function... */

	return true;
}

#else

bool LtCkptPassHeap::predsearch_store(BasicBlock *BB, AliasAnalysis::Location &loc,
                            std::set<BasicBlock *> *visited, StoreInst *inst=NULL)
{

	bool ret = false;
	visited->insert(BB);

	for (BasicBlock::iterator _BI = BB->begin(), _BE = BB->end(); _BI != _BE; ) {
		Instruction* i = _BI++;
		if (isa<StoreInst>(*i)) {
			StoreInst *si = static_cast<StoreInst*>(i);

			/*
			 * It is possible that we are in the original basic block.
			 * For this one we only care to what happens to be before the
			 * store instruction we are currently trying to eliminate instrumentation
			 * for
			 */

			if (inst==i) {
				break;
			}

			AliasAnalysis::Location loc2 = AA->getLocation(si);

			if(loc.Ptr == loc2.Ptr) {
				/* found store in this block don't look further */
				ret = true;
				goto out;
			}
			if(inst->getPointerOperand() == si->getPointerOperand()) {
				/* found store in this block don't look further */
				ret = true;
				goto out;
			}

		} /* isa<Store...  */
	}

	/* the store was not in this block */

	/* check all possible paths */
	for (pred_iterator PI = pred_begin(BB), E = pred_end(BB); PI != E; ++PI) {

		BasicBlock *_B = *PI;

		if (visited->find(_B) != visited->end()) {
			/* however it does if _B does */
			ret = true;
			/* we already had this one */
			continue;
		}

		/* we found at least one viable predecesor... A new Hope... */

		/* Use the force.... */
		ret = predsearch_store(_B, loc, visited,inst);

		if (!ret) {
			/* at least one path does not modfiy loc, give up */
			goto out;
		}
	}

out:

	return ret;
}

bool LtCkptPassHeap::doubleCheckStore(StoreInst *inst)
{
	BasicBlock* BB = inst->getParent();
	AliasAnalysis::Location loc = AA->getLocation(inst);
	std::set<BasicBlock *> visited;

	DT   = &getAnalysis<DominatorTree>(*BB->getParent());

	if (predsearch_store(BB,loc,&visited,inst)) {
		return false;
	}

	return true;
}

#endif



bool LtCkptPassHeap::storeToBeInstrumented(Instruction *inst)
{

	StoreInst &s_inst = *static_cast<StoreInst*>(inst);
	bool ret = LtCkptPassBasic::storeToBeInstrumented(inst);

	if(ret) {
		ret  = doubleCheckStore(&s_inst);
		if (!ret)
			++DoublesRemoved;
		return ret;
	}
	else
		return false;

#if 0
	assert(isa<StoreInst>(*inst));
	Value *ptr  = s_inst.getPointerOperand();

	DSGraph *dsg = EQDS->getDSGraph(*inst->getParent()->getParent());
	assert(dsg);

	DSNodeHandle  dsnh = dsg->getNodeForValue(ptr);
	DSNode *dsn = dsnh.getNode();
	if(!dsn) {
		return true;
		int ret	= doubleCheckStore(&s_inst);
		if (!ret){
			++DoublesRemoved;
		}
		return ret;
	}

	if ( (dsn->isHeapNode() || dsn->isGlobalNode() || dsn->isUnknownNode() || !dsn->isCompleteNode()))
	{
		return true;
		int ret;
		ret = doubleCheckStore(&s_inst);
		if (!ret){
			++DoublesRemoved;
		}
		return ret;
	}
#endif
	return false;
}

LtCkptPassHeap::LtCkptPassHeap():LtCkptPassBasic()
{

}

bool LtCkptPassHeap::runOnModule(Module &M)
{
	EQDS = &getAnalysis<EquivBUDataStructures>();
	return LtCkptPass::runOnModule(M);
}

void LtCkptPassHeap::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.addRequired<DominatorTree>();
    AU.addRequired<AliasAnalysis>();
	AU.addRequired<EQTDDataStructures>();
	LtCkptPass::getAnalysisUsage(AU);
}

char LtCkptPassHeap::ID = 0;

namespace {
	RegisterPass<LtCkptPassHeap> MP("ltckptheap", "Lightweight Checkpointing Pass (Also instrument heap stores)");
}
