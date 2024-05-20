#ifndef LLVM_TRANSFORMS_LOOPINVARIANT_H
#define LLVM_TRANSFORMS_LOOPINVARIANT_H
#include "llvm/Analysis/LoopInfo.h" 
#include <llvm/IR/PassManager.h>
#include <string>
#include <sstream>
#include "llvm/Support/GenericLoopInfo.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"

namespace llvm{
    class LoopInvariant : public PassInfoMixin<LoopInvariant>{
        public:
        PreservedAnalyses run(Loop &L, LoopAnalysisManager &LAM, LoopStandardAnalysisResults &LAR, LPMUpdater &LU);
    };
}
#endif