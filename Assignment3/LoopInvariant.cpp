#include "llvm/Transforms/Utils/LoopInvariant.h"
#include <llvm/IR/PassManager.h>
#include "llvm/IR/Dominators.h"

using namespace llvm;

/**
* Funzione che verifica gli usi di un Value (v) comparandoli all'istruzione attuale(i).
*/
bool definition(Value *v,Instruction &i) {
	for(auto IterU=v->use_begin();IterU!=v->use_end();IterU++){
		if (i.isIdenticalTo(dyn_cast <Instruction>(IterU->getUser()))){
            return true;
        }
	}
    return false;
}
/**
* Funzione che verifica se un'istruzione(i) è un PHINode in quel caso l'istruzione non è LoopInvariant.
*/
bool isPhi(Instruction &i){
    if(PHINode *phiNode = dyn_cast<PHINode>(&i)){
        return true;
    }
    return false;
}
/**
* Funzione che verifica se l'operando (v) di un'istruzione(i) è parametro della funzione. 
*/
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
// Prototipo della funzione per verificare se l'operando di una istruzione la rende LoopInvariant.
bool isLoopInvariantValue(Value*, Loop&, Instruction&);

/*
* Funzione che verifica se un'istruzione(i) all'interno del Loop(L) è LoopInvariant.
*/
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
/*
* Funzione che verifica se gli operandi(v) di un'istruzione(i) non la rendono LoopInvariant. 
*/
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
/*
* Funzione che verifica se il Basic Block in cui è contenuta l'istruzione domina tutte le uscite del Loop.
* Prima sono estratte le uscite del Loop e inserite in uno SmallVector poi una ad una si verifica se sono dominate dal Basic Block dell'istruzione.
*/
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
            outs()<<"non domina le uscite "<<*i<<" "<< block->getName() <<"\n";
            return false;
        }
    }
    return true;
}
/*
* Funzione che verifica se un'istruzione candidata alla code motion domina tutti i suoi usi. Si verificano gli usi e se uno di questi non è dominato ritorna falso.
*/
bool dominatesUse(Instruction* i, DominatorTree& DT, Loop &L){

    for(auto IterU=i->use_begin();IterU!=i->use_end();IterU++){
        if(PHINode *phiNode = dyn_cast<PHINode>(IterU->getUser())){
            //Itera gli incoming value e se il blocco della PHI domina tutti gli incoming block ritorna vero 
            for(auto IterPHI=0 ;IterPHI<phiNode->getNumIncomingValues();IterPHI++){
                if(phiNode->getIncomingValue(IterPHI)== i){
                    BasicBlock *IncomingBlock=phiNode->getIncomingBlock(IterPHI);
                    if(!DT.dominates(i->getParent(),IncomingBlock)){
                        outs() <<"non domina gli gli incoming vlock "<<"\n";
                        return false;
                    }
                }
            }    
        }else if(Instruction* use = dyn_cast <Instruction>(IterU->getUser())){
            if(!DT.dominates(i,use)){
                outs() <<"non domina gli usi " << *use <<"\n";
                return false;
            }
        }
    }
    return true;
}
/*
* Funzione che verifica se un'istruzione è viva solo all'interno del loop controllando i suoi usi. In tal caso è candidata alla code motion.
*/
bool isInstrDead(Instruction *i, Loop  &L){
    for(auto IterU=i->use_begin();IterU!=i->use_end();IterU++){
        if (Instruction* use = dyn_cast <Instruction>(IterU->getUser())){
            if(!L.contains(use->getParent())){
                outs() <<"non dead " << *i <<"\n";
                return false;
            }
        }
    }
    return true;
}

/*
* Funzione che stampa tutte le istruzioni del Loop(L).
*/
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
    DT.print(outs());
    BasicBlock *PreHeader = L.getLoopPreheader();
    //Se il Loop è in forma semplificata si potrà eseguire il Loop Invariant Code Motion.
    if(L.isLoopSimplifyForm()){
        //Si iterano i BB del Loop.
        for(auto IterL=L.block_begin();IterL!=L.block_end();IterL++){
            BasicBlock *BB=*IterL;
            //Si iterano le Instruction in ogni BB.
            for(auto IterI=BB->begin();IterI!=BB->end();IterI++){
                Instruction &i = *IterI;
                outs() << i << "\n";
                //Si verifica se un'istruzione è Loop Invariant. In tal caso sarà aggiunta a uno SmallVector. 
                if (isLoopInvariantInstr(i,L)){
                    invariants.push_back(&i);
                }else{
                    outs() <<" Istruzione non Loop Invariant"<<"\n";
                }
                outs () << "--------------------------------------------"<< "\n";
            }
        }
        /*Si verifica se le istruzioni Loop Invariant sono vive solo all'interno del Loop o se il loro BB domina tutte le uscite e se l'istruzione domina tutti i suoi usi.
        *In tal caso potranno essere messe nel preheader.
        */
        for(auto instr : invariants){
            outs() << *instr <<"\n";
            if(((dominatesExit(instr, DT, L)|| isInstrDead(instr, L))&& dominatesUse(instr,DT,L))){ 
                preHeader.push_back(instr);
            }
        }
        //Sono stampate le istruzioni prima degli spostamenti.
        outs () << " PRIMA --------------------------------------------"<< "\n";
        printInstr(L);
        //Le istruzioni sono spostate nel preheader.
        for(auto instr : preHeader){
            instr->moveBefore(PreHeader->getTerminator());
        }
        //Sono stampate le istruzioni dopo gli spostamenti.
        outs () << " DOPO --------------------------------------------"<< "\n";
        printInstr(L);
    }else{
        outs() <<"Loop in forma non semplificata" <<"\n";
    }
    return PreservedAnalyses::all();
}
