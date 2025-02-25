//
// Created by onioin on 13/02/25.
//

#include "FindCallInst.h"

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
printCallInstructions(raw_ostream &OS, Function &Func,
                         const FindCallInst::Result &CallInsts) noexcept{
    if(CallInsts.empty())
        return;
    OS << "Call instructions in \"" << Func.getName() << "\":\n";

    ModuleSlotTracker Tracker(Func.getParent());

    for(Instruction *Inst : CallInsts){
        Inst->print(OS, Tracker);
        OS << '\n';
    }

}

static constexpr char PassArg[] = "find-call-inst";
static constexpr char PassName[] = "Call instruction locator";
static constexpr char PluginName[] = "FindCallInst";


FindCallInst::Result FindCallInst::run(Function &Func,
                                     FunctionAnalysisManager &FAM){
    Result CallInstructions;
    for(Instruction &Inst : instructions(Func)){
        if(auto *CALL = dyn_cast<CallInst>(&Inst)){
            CallInstructions.push_back(CALL);
            continue;
        }
    }
    return CallInstructions;
}

PreservedAnalyses FindCallInstPrinter::run(Function &Func,
                                          FunctionAnalysisManager &FAM){
    auto &MemOps = FAM.getResult<FindCallInst>(Func);
    printCallInstructions(OS, Func, MemOps);
    return PreservedAnalyses::all();
}

// REGISTER W/ PASS MANAGER

llvm::AnalysisKey FindCallInst::Key;

PassPluginLibraryInfo getFindCallInstPluginInfo(){
    return {LLVM_PLUGIN_API_VERSION, PluginName, LLVM_VERSION_STRING,
            [](PassBuilder &PB){
                PB.registerAnalysisRegistrationCallback(
                        [](FunctionAnalysisManager &FAM) {
                            FAM.registerPass([&] { return FindCallInst(); });
                        });

                PB.registerPipelineParsingCallback(
                        [&](StringRef Name, FunctionPassManager &FPM,
                            ArrayRef<PassBuilder::PipelineElement>) {
                            std::string PrinterPassElement =
                                    formatv("print<{0}>", PassArg);
                            if(!Name.compare(PrinterPassElement)){
                                FPM.addPass(FindCallInstPrinter(llvm::outs()));
                                return true;
                            }
                            return false;
                        });
            }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo(){
    return getFindCallInstPluginInfo();
}

