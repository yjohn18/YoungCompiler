#include <iostream>
#include <cstdio>
#include "parser.h"
#include "abstract_syntax_tree.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

extern std::unique_ptr<llvm::Module> kTheModule;

using namespace std;

void usage() {
    cout << "yc <source file> <target file>" << endl;
}

int main(int argc, char **argv) {

    if (argc != 3) {
        usage();
        return 1;
    }

    Parser p(argv[1]);
    p.MainLoop();

    // Initialize the target registry etc.
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    auto target_triple = llvm::sys::getDefaultTargetTriple();
    kTheModule->setTargetTriple(target_triple);

    string error;
    auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);

    if (!target) {
        llvm::errs() << error;
        return 1;
    }

    auto cpu = "generic";
    auto features = "";

    llvm::TargetOptions opt;
    auto rm = llvm::Optional<llvm::Reloc::Model>();
    auto the_target_machine = 
        target->createTargetMachine(target_triple, cpu, features, opt, rm);

    kTheModule->setDataLayout(the_target_machine->createDataLayout());

    auto filename = argv[2];
    error_code ec;
    llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);

    if (ec) {
        llvm::errs() << "Could not open file: " << ec.message();
        return 1;
    }

    llvm::legacy::PassManager pass;
    auto file_type = llvm::TargetMachine::CGFT_ObjectFile;

    if (the_target_machine->addPassesToEmitFile(pass, dest, nullptr, file_type)) {
        llvm::errs() << "TheTargetMachine can't emit a file of this type";
        return 1;
    }

    pass.run(*kTheModule);
    dest.flush();

    llvm::outs() << "Wrote " << filename << "\n";

    return 0;
}
