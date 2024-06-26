#include "llvm/Transforms/Utils/LoopFusionPass.h"
#include <llvm/IR/PassManager.h>
#include "llvm/IR/Function.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/DependenceAnalysis.h"


using namespace llvm;
/*Controllo di Control Flow Equivalence per verificare dominanza di L1 su L2 e post dominanza di L2 su L1 
usando preheader quando non sono guarded e guard quando sono guarded.*/
bool controlFlowEquivalence(Loop *L1,Loop *L2,Function &F,FunctionAnalysisManager &AM){
    DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
    PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);
    if(L1->isGuarded()){
        if(DT.dominates(L1->getLoopGuardBranch()->getParent(),L2->getLoopGuardBranch()->getParent()) && PDT.dominates(L2->getLoopGuardBranch()->getParent(),L1->getLoopGuardBranch()->getParent())){
            outs()<<"Control Flow equivalence\n";
            return true;
        }else{
            return false;
        }
    }else{
        if(DT.dominates(L1->getLoopPreheader(),L2->getLoopPreheader()) && PDT.dominates(L2->getLoopPreheader(),L1->getLoopPreheader())){
            outs()<<"Control Flow equivalence\n";
            return true;
        }else{
            return false;
        }
    }
    return false;
}
/*Verifica di adiacenza per loop unguarded. Si verifica se l'exit block di L1 è il preheader di L2.*/
bool AreLoopUnguardedAdjacent(Loop *L1, Loop *L2){
    SmallVector <BasicBlock*> exitBB;
    for(auto IterL=L1->block_begin();IterL!=L1->block_end();IterL++){
        BasicBlock *BB=*IterL;
        if(L1->isLoopExiting(BB)){
            outs () << *BB << "\n";
            exitBB.push_back(BB);
        }
    }
    outs() <<"verifica adiacenza\n";
    for(auto IterB2=L2->block_begin();IterB2!=L2->block_end();IterB2++){
        BasicBlock *BB=*IterB2;

    }

    for(auto exit : exitBB){
        outs()<<"entrato nell'if\n";
        BasicBlock *pre=L2->getLoopPreheader();
        if(exit == pre->getUniquePredecessor() ){
            outs() <<"dove inizia uno finisce l'altro\n"; 
            return true;
        }
    }
    return false;
}
/*Verifica di adiacenza per loop guarded. Si verifica se il successore non loop del guard branch di L1 è l'entry block di L2.*/
bool CheckGuard(Loop* L1, Loop* L2){
    SmallVector <BasicBlock*> exitBB;
    BranchInst *bi= L1->getLoopGuardBranch();
    for(auto IterE=0;IterE!= bi->getNumSuccessors();IterE++){
        BasicBlock *BB=bi->getSuccessor(IterE);
        if(!L1->contains(BB)){
            for(auto IterBranch=BB->begin();IterBranch!=BB->end();IterBranch++){
                Instruction &i = *IterBranch;
                if(isa<BranchInst>(i)){
                    for(auto IterE2=0;IterE2!= i.getNumSuccessors();IterE2++){
                        BasicBlock *BB2=i.getSuccessor(IterE2);
                        if(L2->contains(BB2->getSingleSuccessor())){
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}
/*Utilizzando due oggetti SCEVType si verifica se i loop hanno un  numero di iterazioni prevedibile e uguale.*/
bool checkLoopTripCount(Loop* L1, Loop *L2, Function& F, FunctionAnalysisManager &AM){
    ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);
    auto countL1= SE.getBackedgeTakenCount(L1);
    auto countL2= SE.getBackedgeTakenCount(L2);
    if(countL1->getSCEVType()!=SCEVCouldNotCompute().getSCEVType() && countL2->getSCEVType()!=SCEVCouldNotCompute().getSCEVType()){
        if (countL1==countL2){
            outs() << "trip count uguali" <<"\n";
            return true;
        }
    }
    return false;
}
/* Si verifica la presenza di dipendenze negative tra Loop 1 e Loop 2 iterando tutte le istruzione presenti nei loop.*/
bool checkDistanceDependencies(Loop *L1,Loop* L2, Function &F, FunctionAnalysisManager&AM){
    DependenceInfo &DI = AM.getResult<DependenceAnalysis>(F);
    for(auto IterL1=L1->block_begin();IterL1!=L1->block_end();IterL1++){
        BasicBlock *BBL1= *IterL1;
        for(auto IterL1I=BBL1->begin();IterL1I!=BBL1->end();IterL1I++){
            Instruction &iL1= *IterL1I;
            for(auto IterL2=L2->block_begin();IterL2!=L2->block_end();IterL2++){
                BasicBlock *BBL2= *IterL2;
                for(auto IterL2I=BBL2->begin();IterL2I!=BBL2->end();IterL2I++){
                    Instruction &iL2= *IterL2I;
                    auto dep= DI.depends(&iL1,&iL2,true);
                    if(dep){
                        outs() <<" Dipendenza tra \n"<< iL1 <<"\n"<<iL2<<"\n";
                        if(dep->isDirectionNegative()){
                            outs() <<" NEGATIVA \n";
                            return false;
                        }
                    }
                }
            }
        }
        
    }
    return true;
}
/*Vengono uniti i loop connettendo tra loro le seguenti parti:
Unguarded:hL1->exitL2 bodyL1->bodyL2->LatchL1 hL2->LatchL2
Guarded: guardL1->exitL2 headerL1->headerL2->LatchL1->exitL2*/
bool loopFuse(Loop* L1,Loop* L2){
    BasicBlock * headerL1 =L1 -> getHeader();
    BasicBlock * latchL1 =L1 -> getLoopLatch();
    BasicBlock * bodyL1;
    BasicBlock * exitL1= L1 -> getExitBlock();
    BasicBlock * preHeaderL2 = L2-> getLoopPreheader();
    BasicBlock * headerL2 =L2 -> getHeader();
    BasicBlock * latchL2 = L2 -> getLoopLatch();
    BasicBlock * bodyL2;
    BasicBlock * exitL2= L2 -> getExitBlock();
    if(L2->contains(headerL2->getTerminator()->getSuccessor(0))){
        bodyL2 = headerL2->getTerminator()->getSuccessor(0);
    }else{
        bodyL2 = headerL2->getTerminator()->getSuccessor(1);
    }
    if(L1->contains(headerL1->getTerminator()->getSuccessor(0))){
        bodyL1 = headerL1->getTerminator()->getSuccessor(0); 
    }else{
        bodyL1 = headerL1->getTerminator()->getSuccessor(1);
    }
    if(! L1 ->isGuarded()){
        headerL1->getTerminator()->replaceSuccessorWith(preHeaderL2,exitL2);
        bodyL1->getTerminator()->replaceSuccessorWith(latchL1,bodyL2);
        bodyL2->getTerminator()->replaceSuccessorWith(latchL2,latchL1);
        headerL2->getTerminator()->replaceSuccessorWith(bodyL2,latchL2);
    }else{
        BasicBlock * guardL1 =  L1->getLoopGuardBranch()->getParent();
        BasicBlock * guardL2 =  L2->getLoopGuardBranch()->getParent();
        guardL1->getTerminator()->replaceSuccessorWith(guardL2,exitL2);
        headerL1->getTerminator()->replaceSuccessorWith(latchL1,headerL2);
        headerL2->getTerminator()->replaceSuccessorWith(latchL2,latchL1);
        latchL1->getTerminator()->replaceSuccessorWith(exitL1,exitL2);
    }

    return true;
}
//Viene presa l'Induction Variable per entrambi i loop e tutti gli usi della seconda IV sono sostituiti con la prima IV. La seconda viene poi eliminata.
void ChangeIV(Loop* L1, Loop* L2){
    PHINode * IV1 = L1->getCanonicalInductionVariable();
    PHINode * IV2 = L2->getCanonicalInductionVariable();
    IV2->replaceAllUsesWith(IV1);
    IV2->eraseFromParent();
}
//Funzione che prende due loop alla volta dal vettore e verifica se rispettano le condizioni di loop fusion chiamando i metodi di verifica.
bool IsLoopFusionable(SmallVector<Loop *>Loops,Function &F, FunctionAnalysisManager &AM){
    SmallVector<Loop*> LoopV;
    for(auto *L: Loops){
        LoopV.push_back(L);
        outs() << *L <<"\n";
    }
    for(int i=0;i<LoopV.size();i++){
        Loop *L1=LoopV[i];
        outs() << "preso loop 1\n";
        for(int j=i+1;j<LoopV.size();j++){
            Loop *L2=LoopV[j];
            outs() <<"preso loop 2\n";
            //Verifica se sono sullo stesso livello.
            if(L1->getLoopDepth()==L2->getLoopDepth()){
                //Verifica se sono guarded.
                if(L1->isGuarded()){
                    outs() << "loop 1 è guarded\n";
                    //Verifiche per loop guarded.
                    if(CheckGuard(L1,L2) && checkLoopTripCount(L1,L2,F,AM) && controlFlowEquivalence(L1,L2,F,AM) && checkDistanceDependencies(L1,L2,F,AM)){
                        outs() << "I loop sono adiacenti, hanno lo stesso trip count e non hanno dipendenze negative. Procediamo con la fusione\n";
                        //Sostituzione della Induction Variable.
                        ChangeIV(L1,L2);
                        outs() <<"IV sostituita \n";
                        //Metodo per la fusione dei due loop.
                        loopFuse(L1,L2);
                        i+=1;
                    }else{
                        return false;
                    }
                }else{
                    outs() << "loop 1 non è guarded\n";
                    //Verifiche per loop non guarded.
                    if(AreLoopUnguardedAdjacent(L1,L2)  && checkLoopTripCount(L1,L2,F,AM) && controlFlowEquivalence(L1,L2,F,AM) && checkDistanceDependencies(L1,L2,F,AM)){
                        outs() << "I loop sono adiacenti, hanno lo stesso trip count e non hanno dipendenze negative. Procediamo con la fusione\n";
                        //Sostituzione della Induction Variable.
                        ChangeIV(L1,L2);
                        //Metodo per la fusione dei due loop.
                        loopFuse(L1,L2);
                        i+=1;
                    }else{
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

PreservedAnalyses LoopFusionPass::run(Function &F, FunctionAnalysisManager &AM){
    //Estrazione dei loop e inserimento in smallVector che sarà passato alla funzione.
    LoopInfo &LI= AM.getResult<LoopAnalysis>(F);
    SmallVector<Loop*> LoopsOrdered=LI.getLoopsInPreorder();
    IsLoopFusionable(LoopsOrdered,F,AM);
    return PreservedAnalyses::all();
}
