
#include "llvm/ADT/SmallString.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/AsmParser/Parser.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/CodeGen/LinkAllAsmWriterComponents.h"
#include "llvm/CodeGen/LinkAllCodegenComponents.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Linker/Linker.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileUtilities.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/TargetParser/SubtargetFeature.h"
#include "llvm/TargetParser/Triple.h"
#include "llvm/Transforms/Scalar.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <memory>
#include <string>

#include "InjectCacheSim.h"

#include "config.h"

void compile(llvm::Module &M, llvm::StringRef outPath);
void link(llvm::StringRef objFile, llvm::StringRef outFile);

using namespace llvm;

cl::opt<std::string> InPath(cl::Positional, cl::desc("<Module to instrument>"),
                            cl::value_desc("bitcode filename"),
                            cl::init(""), cl::Required,
                            cl::cat(cachePlugin::optionCategory));

cl::opt<std::string> OutFile("o", cl::desc("Output filename"),
                             cl::value_desc("filename"), cl::Required,
                             cl::init(""), cl::cat(cachePlugin::optionCategory));

cl::opt<char> OptLevel("O", cl::desc("Clang optimization level [-O0, -O1, -O2(default), -O3]"),
                      cl::init('2'), cl::Prefix, cl::cat(cachePlugin::optionCategory));

cl::list<std::string> LibPaths("L", cl::Prefix,
                               cl::desc("Specify library search paths"),
                               cl::value_desc("directory"), cl::cat(cachePlugin::optionCategory));

cl::list<std::string> LibNames("l", cl::Prefix,
                               cl::desc("Specify libraries to link against"),
                               cl::value_desc("library"), cl::cat(cachePlugin::optionCategory));

static codegen::RegisterCodeGenFlags cfg;

int main(int argc, char **argv) {
    sys::PrintStackTraceOnErrorSignal(argv[0]);
    llvm::PrettyStackTraceProgram X(argc, argv);
    llvm_shutdown_obj shutdown;
    cl::HideUnrelatedOptions(cachePlugin::optionCategory);
    cl::ParseCommandLineOptions(argc, argv, "Cache Simulator\n\n"
            "   This program injects a basic cache simulator into an llvm IR file.\n");

    SMDiagnostic err;
    LLVMContext CTX;
    std::unique_ptr<Module> M = parseIRFile(InPath.getValue(), err, CTX);

    if(!M.get()){
        errs() << "Error reading bitcode file: " << InPath << "\n";
        err.print(argv[0], errs());
        exit(EXIT_FAILURE);
    }

    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmPrinters();
    InitializeAllAsmParsers();

    llvm::legacy::PassManager pm;
    pm.add(new cachePlugin::CacheSimWrapper());
    pm.add(createVerifierPass());
    pm.run(*M);

    SmallString<32> invocation = llvm::StringRef(argv[0]);
    llvm::sys::path::remove_filename(invocation);
    if(!invocation.empty()){
        LibPaths.push_back(invocation.str().str());
    }
    //this is where the library will be if built correctly
    LibPaths.push_back(invocation.str().str() + "/../cache-sim");

#ifdef CMAKE_INSTALL_PREFIX
    LibPaths.push_back(CMAKE_INSTALL_PREFIX "/cache-sim");
#elif defined(CMAKE_TEMP_LIBRARY_PATH)
    LibPaths.push_back(CMAKE_TEMP_LIBRARY_PATH);
#endif
    LibNames.push_back(RUNTIME_LIB);

    compile(*M, OutFile.getValue() + ".o");
    link(OutFile.getValue() + ".o", OutFile.getValue());

    exit(EXIT_SUCCESS);

}

void compile(Module &M, StringRef outPath){
    std::string err;
    Triple triple = Triple(M.getTargetTriple());
    Target const* target =
            TargetRegistry::lookupTarget(codegen::getMArch(), triple, err);
    if(!target){
        report_fatal_error(Twine("Unable to find target:\n" + err));
    }

    CodeGenOptLevel lvl = CodeGenOptLevel::Default;

    switch (OptLevel) {
        default: report_fatal_error("Invalid optimization level.\n");
        case '0': lvl = CodeGenOptLevel::None; break;
        case '1': lvl = CodeGenOptLevel::Less; break;
        case '2': lvl = CodeGenOptLevel::Default; break;
        case '3': lvl = CodeGenOptLevel::Aggressive; break;
    }

    TargetOptions opts = llvm::codegen::InitTargetOptionsFromCodeGenFlags(triple);
    auto relocModel = llvm::codegen::getExplicitRelocModel();
    auto codeModel = llvm::codegen::getExplicitCodeModel();
    if(!relocModel){
        relocModel = llvm::Reloc::Model::PIC_;
    }

    std::unique_ptr<TargetMachine> machine(
            target->createTargetMachine(triple.getTriple(),
                                        codegen::getCPUStr(),
                                        codegen::getFeaturesStr(),
                                        opts,
                                        relocModel,
                                        codeModel,
                                        lvl));
    assert(machine && "No target machine :(");

    if(llvm::codegen::getFloatABIForCalls() != FloatABI::Default){
        opts.FloatABIType = llvm::codegen::getFloatABIForCalls();
    }

    std::error_code errc;
    auto out =
            std::make_unique<ToolOutputFile>(outPath, errc, sys::fs::OF_None);
    if(!out) {
        report_fatal_error(Twine("Unable to create file:\n" + errc.message()));
    }

    llvm::legacy::PassManager pm;
    TargetLibraryInfoImpl tlii(triple);
    pm.add(new TargetLibraryInfoWrapperPass(tlii));
    M.setDataLayout(machine->createDataLayout());

    {
        llvm::raw_pwrite_stream* os(&out->os());
        std::unique_ptr<buffer_ostream> bos;
        if(!out->os().supportsSeeking()){
            bos = std::make_unique<buffer_ostream>(*os);
            os = bos.get();
        }

        if(machine->addPassesToEmitFile(pm, *os,
                                        nullptr, CodeGenFileType::ObjectFile)){
            report_fatal_error("cannot generate a file of this type");
        }
        cl::PrintOptionValues();
        pm.run(M);
    }
    out->keep();

}

void link(StringRef objFile, StringRef outFile){
    auto clang = llvm::sys::findProgramByName("clang++");
    std::string opt("-O" + OptLevel);
    if(!clang){
        report_fatal_error("No clang :(");
    }

    std::vector<std::string> args{clang.get(), opt, "-o", outFile.str(), objFile.str()};

    for(auto & libPath : LibPaths){
        args.push_back("-L" + libPath);
    }
    for(auto & lib : LibNames){
        args.push_back("-l" + lib);
    }

    std::vector<llvm::StringRef> charArgs;
    charArgs.reserve(args.size());

    for(auto & arg : args){
        charArgs.emplace_back(arg);
    }
    for(auto & arg : args){
        outs() << arg.c_str() << " ";
    }
    outs() << "\n";

    auto res = llvm::sys::ExecuteAndWait(clang.get(), charArgs);

    if(-1 == res){
        report_fatal_error("Unable to link");
    }

}

