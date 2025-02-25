
#ifndef LLVM_FIND_MEM_H
#define LLVM_FIND_MEM_H

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include <vector>

namespace llvm{

class Instruction;
class Function;
class Module;
class raw_ostream;

}

// ---------------------------------------

class FindMemCall : public llvm::AnalysisInfoMixin<FindMemCall> {
public:
  using Result = std::vector<llvm::Instruction *>;
  Result run(llvm::Function &Func, llvm::FunctionAnalysisManager &FAM);

private:
  friend struct llvm::AnalysisInfoMixin<FindMemCall>;
  static llvm::AnalysisKey Key;
};

// -------------------------------------

class FindMemCallPrinter : public llvm::PassInfoMixin<FindMemCallPrinter>{
public:
  explicit FindMemCallPrinter(llvm::raw_ostream &OutStream) : OS(OutStream){};

  llvm::PreservedAnalyses run(llvm::Function &Func,
                              llvm::FunctionAnalysisManager &FAM);
private:
  llvm::raw_ostream &OS;

};


#endif
