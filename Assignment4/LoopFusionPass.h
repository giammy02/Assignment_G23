#ifndef LLVM_TRANSFORMS_LOOPFUSIONPASS_H
#define LLVM_TRANSFORMS_LOOPFUSIONPASS_H
#include "llvm/Analysis/LoopInfo.h" 
#include <llvm/IR/PassManager.h>
#include <string>
#include <sstream>
#include "llvm/Support/GenericLoopInfo.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"


namespace llvm{
    class LoopFusionPass : public PassInfoMixin<LoopFusionPass>{
        public:
        PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
    };
}
#endif