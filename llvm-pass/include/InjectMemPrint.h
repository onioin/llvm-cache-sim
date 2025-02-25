

#ifndef LLVM_INJECT_MEM_PRINT_H
#define LLVM_INJECT_MEM_PRINT_H

#include "FindMemCall.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"


struct InjectMemPrint : llvm::PassInfoMixin<InjectMemPrint>{

  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &);

  bool runOnModule(llvm::Module &M);

  static bool isRequired() {return true;}

};

#endif
