#ifndef LLVM_TRANSFORMS_LOCALOPTS_H
#define LLVM_TRANSFORMS_LOCALOPTS_H
#include <llvm/IR/Constants.h>
#include <llvm/IR/PassManager.h>
namespace llvm{
	class LocalOpts : public  PassInfoMixin<LocalOpts>{
		public:
			PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
	};
}
#endif
