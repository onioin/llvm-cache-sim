

#include "FindMemCall.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"

#include <string>

using namespace llvm;

static void
    printMemCallInstructions(raw_ostream &OS, Function &Func,
                        const FindMemCall::Result &MemInsts) noexcept{
  if(MemInsts.empty())
    return;
  OS << "Memory instructions in \"" << Func.getName() << "\":\n";

  ModuleSlotTracker TrackerLD(Func.getParent());
  ModuleSlotTracker TrackerST(Func.getParent());

  for(Instruction *Inst : MemInsts){
    if(auto *LD = dyn_cast<LoadInst>(Inst)){
      LD->print(OS, TrackerLD);
      OS << '\n';
      continue ;
    }
    if(auto *ST = dyn_cast<StoreInst>(Inst)){
      ST->print(OS, TrackerST);
      OS << '\n';
    }
  }

}

static constexpr char PassArg[] = "find-mem-call";
static constexpr char PassName[] = "Memory operations locator";
static constexpr char PluginName[] = "FindMemCall";


FindMemCall::Result FindMemCall::run(Function &Func,
                                     FunctionAnalysisManager &FAM){
  Result MemInstructions;
  for(Instruction &Inst : instructions(Func)){
    if(auto *LD = dyn_cast<LoadInst>(&Inst)){
      MemInstructions.push_back(LD);
      continue;
    }
    if(auto *ST = dyn_cast<StoreInst>(&Inst)){
      MemInstructions.push_back(ST);
    }
  }
  return MemInstructions;
}

PreservedAnalyses FindMemCallPrinter::run(Function &Func,
                                          FunctionAnalysisManager &FAM){
  auto &MemOps = FAM.getResult<FindMemCall>(Func);
  printMemCallInstructions(OS, Func, MemOps);
  return PreservedAnalyses::all();
}

// REGISTER W/ PASS MANAGER

llvm::AnalysisKey FindMemCall::Key;

PassPluginLibraryInfo getFindMemCallPluginInfo(){
  return {LLVM_PLUGIN_API_VERSION, PluginName, LLVM_VERSION_STRING,
          [](PassBuilder &PB){
          PB.registerAnalysisRegistrationCallback(
              [](FunctionAnalysisManager &FAM) {
                FAM.registerPass([&] { return FindMemCall(); });
              });

          PB.registerPipelineParsingCallback(
              [&](StringRef Name, FunctionPassManager &FPM,
                  ArrayRef<PassBuilder::PipelineElement>) {
                std::string PrinterPassElement =
                  formatv("print<{0}>", PassArg);
                if(!Name.compare(PrinterPassElement)){
                  FPM.addPass(FindMemCallPrinter(llvm::outs()));
                  return true;
                }
                return false;
              });
        }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo(){
  return getFindMemCallPluginInfo();
}
