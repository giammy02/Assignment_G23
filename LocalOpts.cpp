//===-- LocalOpts.cpp - Example Transformations --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/LocalOpts.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Support/PointerLikeTypeTraits.h"
#include "llvm/IR/IRBuilder.h"
#include <string>
#include <sstream>
using namespace llvm;

/*
-------------------------IDENTITA' ALGEBRICA MUL-------------------------
la funzione viene chiamata se viene trovato 1 in una moltiplicazione 
o 0 in un'addizione e ci si trova davanti a un caso di identità algebrica 
a=b*1=1*b->a=b oppure a=b+0=0+b->a=b quindi possiamo sostituire 
l'operazione in assegnamento.*/
/*
-------------------------MULTI INSTRUCTION OPTIMIZATION-------------------
la funzione viene chiamata se viene trovata una sottrazione tra una variabile(minuendo) e una 
costante(sottraendo) e la variabile viene creata da una addizione tra una 
variabile e una costante si verifica se le costanti di addizione e sottrazioni
sono equivalenti per trasformare la sottrazione in un assegnamento.
*/
void create_assign(Instruction &i,Value *operand, int nOperand){
	/*è necessario il tipo degli operandi per poter creare un assegnamento adeguato.*/
	Type* opType=operand->getType();
	/*creazione di un'istruzione alloca per allocare lo spazio necessario sullo stack frame della
	funzione attualmente in esecuzione, il tipo dell'operando servirà per definire quanto spazio allocare,
	si usa l'operando numero nOp della istruzione instr, l'istruzione sara' inserita dopo instr. */
	AllocaInst *alloc= new AllocaInst(opType, NULL, i.getOperand(nOperand),Twine(*"hw"), &i);
	/*creazione di un'istruzione per scrivere in memoria l operando numero nOp puntato dall'istruzione alloc
	l'istruzione sarà inserita dopo instr.*/
	StoreInst *s=new StoreInst(i.getOperand(nOperand),alloc,&i);
	/*creazione di un'istruzione per leggere dalla memoria un valore di tipo inType che si trova in alloc. Questa
	istruzione viene inserita dopo instr.*/
	LoadInst *l=new LoadInst(opType,alloc,Twine(*"v"),&i);
	//tutti gli usi di i sono sostituiti da l
	i.replaceAllUsesWith(l);
}
/*
-------------------------STRENGTH REDUCTION MUL-------------------------
la funzione è chiamata se la variabile APInt è una potenza di 2. In questo
caso è possibile applicare la strength reduction per cui a=b*16 -> a<<4 
oppure a/16=a>>4 oppure a*15=a<<4; a-=a; oppure a*17= a<<4;a+=a in questo 
modo si utilizza un'operazione di shift e se necessaria un'addizione o una
sottrazione invece che una moltiplicazione.*/
void create_shift(Instruction &i, Value * operand, int operation, APInt val, int nOp){
	/*è necessario il tipo degli operandi per poter creare variabili adeguate.*/
	Type* opType=operand->getType();
	/*oggetti di tipo istruzione per sostituire moltiplicazioneo o divisione.*/
	Instruction* newInst;
	Instruction* newInst2; 
	/*variabile in cui dovrà essere inserito il valore costante.*/
	Value *v;
	/*costante in cui sarà inserito il valore per cui shiftare la variabile.*/
	Constant *C2;
	switch(operation){
		case 1:
			C2=ConstantInt::get(opType,val.logBase2(),false);
			//si crea il Value per poterlo inserire nella nuova istruzione.
			v=C2;
			/*si crea una nuova istruzione shift left che utilizza come rvalue il registro
			che sara' poi shiftato di v posizioni.*/
			newInst = BinaryOperator::Create(Instruction::Shl, i.getOperand(nOp),v);
			/*la nuova istruzione viene inserita dopo quella vecchia, gli usi della vecchia istruzione
			sono rimpiazzati.*/
			newInst->insertAfter(&i);
			//gli usi della vecchia istruzione saranno sostituiti con il risultato dello shift.
			i.replaceAllUsesWith(newInst);
			break;
		case 2:
			C2=ConstantInt::get(opType,val.logBase2(),false);
			v=C2;
			newInst= BinaryOperator::Create(Instruction::Shl, i.getOperand(nOp),v);
			//creazione di una seconda nuova istruzione(sottrazione).
			newInst2= BinaryOperator::Create(Instruction::Sub, newInst,i.getOperand(nOp));
			newInst->insertAfter(&i);
			newInst2->insertAfter(newInst);
			//gli usi della vecchia istruzione sono sostituiti con il risultato della sottrazione.
			i.replaceAllUsesWith(newInst2);
			break;
		case 3:
			C2=ConstantInt::get(opType,val.logBase2(),false);
			v=C2;
			newInst= BinaryOperator::Create(Instruction::Shl, i.getOperand(nOp),v);
			//creazione di una seconda nuova istruzione(addizione).
			newInst2= BinaryOperator::Create(Instruction::Add, newInst,i.getOperand(nOp));
			newInst->insertAfter(&i);
			newInst2->insertAfter(newInst);
			//gli usi della vecchia istruzione sono sostituiti con il risultato della addizione.
			i.replaceAllUsesWith(newInst2);
			break;
		case 4:
			C2=ConstantInt::get(opType,val.logBase2(),false);
			v=C2;
			//viene creato un logic shift right per la divisione
			newInst = BinaryOperator::Create(Instruction::LShr, i.getOperand(0),v);
			newInst->insertAfter(&i);
			//gli usi della vecchia istruzione sono sostituiti con il risultato della divisione.
			i.replaceAllUsesWith(newInst);
			break;
	}
}

bool runOnBasicBlock(BasicBlock &B) {
	/*vettore in cui vengono aggiunte le istruzioni da rimuovere dopo aver navigato tutte le istruzioni del BB
	e (per quelle idonee alla rimozione) aver rimpiazzato tutti gli usi con le  nuove istruzioni.*/
	std::vector<Instruction*> del;
	/*ciclo for che itera le istruzioni del BB utilizzando Iter il cui valore puntato sara' assegnato a un
	oggetto di tipo Instruction.*/
	for(auto Iter=B.begin(); Iter!=B.end();Iter++){
		//viene presa l'istruzione in cui ci si trova in questo momento.
		Instruction &instr=*Iter;
		/*variabile intera utilizzata per definire quale operando dell'istruzione dovrà essere usato nelle
		nuove istruzioni.*/
		int nOp=1;
		//verifica se l'istruzione attuale è una moltiplicazione per poter eseguire ottimizzazioni.
		if(instr.getOpcodeName()=="mul"){
			/*ciclo for che itera sugli operatori dell'istruzione, ogni operatore è assegnato a un Value e
			poi si tenta il casting che se è positivo permette di verificare se l'istruzione è ottimizzabile.*/
			for(auto *IterI= instr.op_begin(); IterI!=instr.op_end();IterI++){
				//viene preso l'operando in cui ci si trova in questo momento.
				Value *operand= *IterI;
				/*si tenta il casting dell'operando in caso si riuscisse a fare partiranno i controlli
				per le operazioni di ottimizzazione.*/
				if(ConstantInt *C = dyn_cast <ConstantInt>(operand)){
					//estrazione del valore dalla costante.
					APInt i=C->getValue();
					//se la costante ha valore 1 ci troviamo in un caso di identità algebrica.
					if(i.isOne()){
						create_assign(instr, operand, nOp);
						//la vecchia istruzione viene messa nel vettore del 
						del.push_back(&instr);
					}
					//se la costante ha come valore una potenza di due possiamo applicare la strength reduction.
					else if (i.isPowerOf2()){
						create_shift(instr,operand,1,i,nOp);
						del.push_back(&instr);
					}
					else{
						//si modifica il valore della costante per verificare se è vicino a una potenza di 2.
						i+=1;
						if (i.isPowerOf2()){
							create_shift(instr,operand,2,i,nOp);
							del.push_back(&instr);
						}
						//si modifica il valore della costante per verificare se è vicino a una potenza di 2.
						i-=2;
						if (i.isPowerOf2()){
							create_shift(instr,operand,3,i,nOp);
							del.push_back(&instr);
						}
					}

				}
				nOp=0;
			}
		}
		//verifica se l'istruzione attuale è una divisione.
		else if(instr.getOpcodeName()=="sdiv"){
			//itera gli operatori e assegna a operand il secondo operatore
			for(auto *IterI= instr.op_begin(); IterI!=instr.op_end();IterI++){
				IterI++;
				Value *operand= *IterI;
				//si tenta il casting di operand in ConstantInt
				if(ConstantInt *C = dyn_cast <ConstantInt>(operand)){
					APInt i=C->getValue();
					//se la costante ha come valore una potenza di due possiamo applicare la strength reduction.
					if (i.isPowerOf2()){
						create_shift(instr,operand,4,i,nOp);
					    del.push_back(&instr);
					}
				}
			}
		}
		//verifica se l'istruzione attuale è un'addizione.
		else if (instr.getOpcodeName()=="add"){
			//itera gli operatori e assegna ad operand l'operatore attuale
			for(auto *IterI=instr.op_begin();IterI!=instr.op_end();IterI++){
				Value *operand= *IterI;
				//si tenta il casting di operand in ConstantInt
				if(ConstantInt *C = dyn_cast <ConstantInt>(operand)){
					APInt i=C->getValue();
					//se la costante ha come valore 0 possiamo trasformarlo in assegnamento.
					if (i.isZero()){
						create_assign(instr, operand, nOp);
						del.push_back(&instr);
					}
				}
				nOp=0;
			}
		}
	}
	//le istruzioni aggiunte al vettore del vengono rimosse dal BB. 
    for (auto inst : del){
		inst->eraseFromParent();
	}
	//definizione di un nuovo vettore in cui inserire le nuove istruzioni da rimuovere.
	std::vector <Instruction*> del2;
	/*ciclo for per rinavigare il BB per verificare la presenza di situazioni di Multi Instruction Optimization
	dopo aver risolto situazioni di strength reduction e algebraic identities.
    esempio: a=b+1; c=a-1;-> a=b+1*/
	for (auto Iter=B.begin();Iter!=B.end();Iter++){
		Instruction &instr=*Iter;
		//verifica se l'istruzione attuale è una sottrazione.
		if (instr.getOpcodeName()=="sub"){
			Value *operand, *op2;
			int val1,val2;
			outs()<<"SOTTRAZIONE"<<"\n";
			//estrazione di entrambi gli operandi della sottrazione.
			for(auto *IterI= instr.op_begin(); IterI!=instr.op_end();IterI++){
				operand= *IterI;
				IterI++;
				op2=*IterI;
			}
			/*tentativo di casting del secondo operando(sottraendo), in caso
			di successo si potrà verificare se l'origine del minuendo è una
			addizione utilizzando gli indirizzi delle operazioni(si estrae dal 
			primo operando l'indirizzo tramite uno stringstream che poi verrà
			inserito in una stringa).*/
			if (ConstantInt *C2 = dyn_cast <ConstantInt>(op2)){
				val2=C2->getSExtValue();
				Value *v=operand;
				std::ostringstream addrSub;
				addrSub<<v;
				std::string s1= addrSub.str();
				/*si naviga un'ultima volta nel BB fino all'istruzione in cui ci troviamo ora
				(sottrazione).*/
				for (auto IterC=B.begin();IterC!=Iter;IterC++){
					Instruction &ins=*IterC;
					/*si verifica se si trova una addizione e da essa si estrae l'indirizzo per
					confrontarlo con l'indirizzo estratto dal primo operando della sottrazione.*/
					if (ins.getOpcodeName()=="add"){
						int con=1;
						std::ostringstream addrAdd;
						addrAdd<<&ins;
						std::string s2= addrAdd.str();
						//se i due indirizzi combaciano si può procedere con la trasformazione.
						if (s1==s2){
							/*vengono estratti gli operandi della addizione e si verifica se le 
							costanti di addizione e sottrazione combaciano in caso positivo si 
							può trasformare la sottrazione in un assegnamento.*/
							for (auto *IterAdd=ins.op_begin();IterAdd!=ins.op_end();IterAdd++){
								Value *opAdd=*IterAdd;
								if(ConstantInt *CADD= dyn_cast<ConstantInt>(opAdd)){
									int vAdd=CADD->getSExtValue();
									if(vAdd==val2){
										Type* inType= opAdd->getType();
										AllocaInst *alloc= new AllocaInst(inType, NULL,ins.getOperand(con),Twine(*"hw"), &instr);
										StoreInst *s=new StoreInst(instr.getOperand(con),alloc,&instr);
										LoadInst *l=new LoadInst(inType,alloc,Twine(*"v"),&instr);
										instr.replaceAllUsesWith(l);
										del2.push_back(&instr);
									}
								}else{
									con=0;
								}
							}
						}
					}
				}
			}
		}
	}
    //se il nuovo vettore non è vuoto si procede con l'eliminazione delle istruzioni che si trovano al suo interno dal BB.
    if (!del2.empty()){
	    for (auto inst : del2){
			inst->eraseFromParent();
	    	}
		}
    return true;
  }

bool runOnFunction(Function &F) {
  bool Transformed = false;

  for (auto Iter = F.begin(); Iter != F.end(); ++Iter) {
    if (runOnBasicBlock(*Iter)) {
      Transformed = true;
    }
  }

  return Transformed;
}


PreservedAnalyses LocalOpts::run(Module &M,
                                      ModuleAnalysisManager &AM) {
  for (auto Fiter = M.begin(); Fiter != M.end(); ++Fiter)
    if (runOnFunction(*Fiter))
      return PreservedAnalyses::none();
  
  return PreservedAnalyses::all();
}
