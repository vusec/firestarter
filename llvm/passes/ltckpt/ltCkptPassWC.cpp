#include "ltckpt/ltCkptPassWC.h"

bool LtCkptPassWC::memIntrinsicToBeInstrumented(Instruction *inst)
{
	return true;
}

bool LtCkptPassWC::storeToBeInstrumented(Instruction *inst)
{
	return true;
}

LtCkptPassWC::LtCkptPassWC():LtCkptPassBasic() 
{
}

char LtCkptPassWC::ID = 0;

RegisterPass<LtCkptPassWC> X("ltckptwc", "Leightweight Checkpointing Pass (Worst Case Instrumentation)");
