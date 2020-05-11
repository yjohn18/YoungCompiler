#include "tools.h"
#include <iostream>

std::unique_ptr<ExprAst> LogError(const char *str) {
    std::cerr << "Error: " << str << std::endl;
    return nullptr;
}

std::unique_ptr<PrototypeAst> LogErrorP(const char *str) {
    LogError(str);
    return nullptr;
}

llvm::Value *LogErrorV(const char *str) {
    LogError(str);
    return nullptr;
}



