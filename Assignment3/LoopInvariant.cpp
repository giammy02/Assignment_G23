#include "llvm/Transforms/Utils/LoopInvariant.h"
#include <llvm/IR/PassManager.h>
#include "llvm/IR/Dominators.h"

using namespace llvm;

bool definition(Value *v,Instruction &i) {
	for(auto IterU=v->use_begin();IterU!=v->use_end();IterU++){
		if (i.isIdenticalTo(dyn_cast <Instruction>(IterU->getUser()))){
            return true;
        }
	}
    return false;
}
bool isPhi(Instruction &i){
    if(PHINode *phiNode = dyn_cast<PHINode>(&i)){
        return true;
    }
    return false;
}
bool checkArgs(Value* v,Loop &L,Instruction &i){
    Function *F=(L.getHeader())->getParent();
	for(auto IterArg=F->arg_begin();IterArg!=F->arg_end();IterArg++){
        Value *arg=IterArg;
        if(definition(arg,i)){
            return true;
        }
    }
    return false;
}
bool isLoopInvariantValue(Value*, Loop&, Instruction&);

bool isLoopInvariantInstr(Instruction &i, Loop &L){
    if( isPhi(i)){
        return false;
    }else{
        for (auto *IterO=i.op_begin();IterO!=i.op_end();IterO++){
            Value *Operand= *IterO;
            if(isLoopInvariantValue(Operand, L, i)==false){
                return false;
            }
        }
    }
    return true;
}
bool isLoopInvariantValue(Value* v, Loop &L, Instruction &i){
    if (checkArgs(v,L,i)){
        return true;
    }
    if (Instruction *inst = dyn_cast<Instruction>(v)){
        outs() << *inst <<"\n";
        if(!L.contains(inst)){
            outs() <<"istruzione fuori"<<"\n";
            return true;
        }else{
            outs() <<"altra istruzione"<<"\n";
            if(isLoopInvariantInstr(*inst, L)==false){
                return false;
            }else{
                return true;
            }
        }
    }
    else if(Constant *C = dyn_cast<Constant>(v)){
        outs()<<"costante!"<<"\n";
        return true;
    }
    return false;
}

bool dominatesExit(Instruction* i,DominatorTree& DT, Loop& L){
    SmallVector <BasicBlock*> exitBB;
    for(auto IterL=L.block_begin();IterL!=L.block_end();IterL++){
        BasicBlock *BB=*IterL;
        if(L.isLoopExiting(BB)){
            exitBB.push_back(BB);
        }
    }
    for(auto block : exitBB){
        if(!DT.dominates(i->getParent(),block)){
            return false;
        }
    }
    return true;
}

bool dominatesUse(Instruction* i, DominatorTree& DT){

    for(auto IterU=i->use_begin();IterU!=i->use_end();IterU++){
        if(Instruction* use = dyn_cast <Instruction>(IterU->getUser())){
            if(!DT.dominates(i,use)){
                return false;
            }
        }
    }
    return true;
}

bool isInstrDead(Instruction *i, Loop  &L){
    for(auto IterU=i->use_begin();IterU!=i->use_end();IterU++){
        if (Instruction* use = dyn_cast <Instruction>(IterU->getUser())){
            if(!L.contains(use->getParent())){
                return false;
            }
        }
    }
    return true;
}

void printInstr(Loop& L){
    for(auto IterL=L.block_begin();IterL!=L.block_end();IterL++){
        BasicBlock *BB=*IterL;
        for(auto IterI=BB->begin();IterI!=BB->end();IterI++){
            Instruction &i = *IterI;
            outs() << i << "\n";
        }
    }
}

PreservedAnalyses LoopInvariant::run(Loop &L, LoopAnalysisManager &LAM, LoopStandardAnalysisResults &LAR, LPMUpdater &LU){
    SmallVector <Instruction*> invariants, preHeader;
    DominatorTree &DT = LAR.DT;
    BasicBlock *PreHeader = L.getLoopPreheader();
    for(auto IterL=L.block_begin();IterL!=L.block_end();IterL++){
        BasicBlock *BB=*IterL;
        for(auto IterI=BB->begin();IterI!=BB->end();IterI++){
            Instruction &i = *IterI;
            outs() << i << "\n";
            if (isLoopInvariantInstr(i,L)){
                invariants.push_back(&i);
            }else{
                outs() <<" Istruzione non Loop Invariant"<<"\n";
            }
            outs () << "--------------------------------------------"<< "\n";
        }
	}
    for(auto instr : invariants){
        outs() << instr <<"\n";
        if((dominatesExit(instr, DT, L) || isInstrDead(instr, L))&& dominatesUse(instr,DT)){ 
            preHeader.push_back(instr);
        }
    }
     outs () << " PRIMA --------------------------------------------"<< "\n";
    printInstr(L);
    for(auto instr : preHeader){
        instr->moveBefore(PreHeader->getTerminator());
    }
     outs () << " DOPO --------------------------------------------"<< "\n";
     printInstr(L);
    return PreservedAnalyses::all();
}
