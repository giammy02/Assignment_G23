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
using namespace llvm;

bool runOnBasicBlock(BasicBlock &B) {
	SmallVector<llvm::Instruction*> del;
	for(auto Iter=B.begin(); Iter!=B.end();Iter++){
		Instruction &instr=*Iter;
		int nOp=1;
		outs() << "\t" << instr.getOpcodeName() << "\n";
		if(instr.getOpcodeName()=="mul"){
			for(auto *IterI= instr.op_begin(); IterI!=instr.op_end();IterI++){
				Value *operand= *IterI;
				if(ConstantInt *C = dyn_cast <ConstantInt>(operand)){
					outs() <<nOp<< "\n";
					APInt i=C->getValue();
					if(i.isOne()){
						Type* inType= operand->getType();
						AllocaInst *alloc= new AllocaInst(inType, NULL,instr.getOperand(nOp),Twine(*"hw"), &instr);
						StoreInst *s=new StoreInst(instr.getOperand(nOp),alloc,&instr);
						LoadInst *l=new LoadInst(inType,alloc,Twine(*"v"),&instr);
						instr.replaceAllUsesWith(l);
						del.push_back(&instr);
					}
					else if (i.isPowerOf2()){
						Type* inType= operand->getType();
						Constant *C2=ConstantInt::get(inType,i.logBase2(),false);
						Value *v=C2;
						Instruction *newInst = BinaryOperator::Create(Instruction::Shl, instr.getOperand(0),v);
						newInst->insertAfter(&instr);
						instr.replaceAllUsesWith(newInst);
						del.push_back(&instr);
					}
					i+=1;
					if (i.isPowerOf2()){
						Type* inType=operand->getType();
						Constant *C2=ConstantInt::get(inType,i.logBase2(),false);
						Value *v=C2;
						Instruction *newInst= BinaryOperator::Create(Instruction::Shl, instr.getOperand(0),v);
						Instruction *newInst2= BinaryOperator::Create(Instruction::Sub, newInst,instr.getOperand(0));
						newInst->insertAfter(&instr);
						newInst2->insertAfter(newInst);
						instr.replaceAllUsesWith(newInst2);
						del.push_back(&instr);
					}
					i-=2;
					if (i.isPowerOf2()){
						Type* inType=operand->getType();
						Constant *C2=ConstantInt::get(inType,i.logBase2(),false);
						Value *v=C2;
						Instruction *newInst= BinaryOperator::Create(Instruction::Shl, instr.getOperand(0),v);
						Instruction *newInst2= BinaryOperator::Create(Instruction::Add, newInst,instr.getOperand(0));
						newInst->insertAfter(&instr);
						newInst2->insertAfter(newInst);
						instr.replaceAllUsesWith(newInst2);
						del.push_back(&instr);
					}

				}
				nOp=0;
			}
		}
		else if(instr.getOpcodeName()=="sdiv"){
			for(auto *IterI= instr.op_begin(); IterI!=instr.op_end();IterI++){
				Value *operand= *IterI;
				if(ConstantInt *C = dyn_cast <ConstantInt>(operand)){
					outs() <<"costante!"<< "\n";
					APInt i=C->getValue();
					if (i.isPowerOf2()){
						Type* inType= operand->getType();
						Constant *C2=ConstantInt::get(inType,i.logBase2(),false);
						Value *v=C2;
						Instruction *newInst = BinaryOperator::Create(Instruction::LShr, instr.getOperand(0),v);
						newInst->insertAfter(&instr);
						instr.replaceAllUsesWith(newInst);
						del.push_back(&instr);
					}
				}
			}
		}
		else if (instr.getOpcodeName()=="add"){
			for(auto *IterI=instr.op_begin();IterI!=instr.op_end();IterI++){
				Value *operand= *IterI;
				if(ConstantInt *C = dyn_cast <ConstantInt>(operand)){
					outs() <<nOp<< "\n";
					APInt i=C->getValue();
					if (i.isZero()){
						Type* inType= operand->getType();
						AllocaInst *alloc= new AllocaInst(inType, NULL,instr.getOperand(nOp),Twine(*"hw"), &instr);
						StoreInst *s=new StoreInst(instr.getOperand(nOp),alloc,&instr);
						LoadInst *l=new LoadInst(inType,alloc,Twine(*"v"),&instr);
						instr.replaceAllUsesWith(l);
						del.push_back(&instr);
					}
				}
				nOp=0;
			}
		}
     }
    for (auto inst : del){
	inst->eraseFromParent();
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
