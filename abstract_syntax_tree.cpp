#include "abstract_syntax_tree.h"
#include "tools.h"
#include <vector>
#include <iostream>

using namespace llvm;

Value *NumberExprAst::CodeGen() {
    return ConstantFP::get(the_context_, APFloat(val_));
}

Value *VariableExprAst::CodeGen() {
    Value *V = named_values_[name_];

    if (!V)
        LogErrorV("Unknown variable name");

    return V;
}

Value *BinaryExprAst::CodeGen() {
    Value *l = lhs_->CodeGen();
    Value *r = rhs_->CodeGen();
    if (!l || !r)
        return nullptr;

    switch (op_) {
    case '+':
        return (builder_).CreateFAdd(l, r, "addtmp");
    case '-':
        return builder_.CreateFSub(l, r, "subtmp");
    case '*':
        return builder_.CreateFMul(l, r, "multmp");
    case '<':
        l = builder_.CreateFCmpULT(l, r, "cmptmp");
        return builder_.CreateUIToFP(l, Type::getDoubleTy(the_context_), "booltmp");
    default:
        return LogErrorV("invalid binary operator");
    }
}

Value *CallExprAst::CodeGen() {
    Function *callee_f = the_module_->getFunction(callee_);
    if (!callee_f)
        return LogErrorV("Unknown function referenced");

    if (callee_f->arg_size() != args_.size())
        return LogErrorV("Incorrect # arguments passed");

    std::vector<Value *> args_v;
    for (unsigned i = 0, e = args_.size(); i != e; ++i) {
        args_v.push_back(args_[i]->CodeGen());
        if (!args_v.back())
            return nullptr;
    }

    return builder_.CreateCall(callee_f, args_v, "calltmp");
}

Function *PrototypeAst::CodeGen() {
    std::vector<Type *> doubles(args_.size(), Type::getDoubleTy(the_context_));

    FunctionType *ft = FunctionType::get(Type::getDoubleTy(the_context_), doubles, false);

    Function *f = Function::Create(ft, Function::ExternalLinkage, name_, the_module_.get());

    // Set names for all arguments.
    unsigned idx = 0;
    for (auto &arg : f->args())
        arg.setName(args_[idx++]);

    return f;
}

Function *FunctionAst::CodeGen() {
    // First check for an existing function from a previous "extern" declaration.
    Function *the_function = the_module_->getFunction(proto_->name());

    if (!the_function)
        the_function = proto_->CodeGen();

    if (!the_function)
        return nullptr;

    if (!the_function->empty())
        return (Function*)LogErrorV("Function cannot be redefined.");

    // Create a new basic block to start insertion into.
    BasicBlock *bb  = BasicBlock::Create(the_context_, "entry", the_function);
    builder_.SetInsertPoint(bb);

    // Record the function arguments in the NamedValues map.
    named_values_.clear();
    for (auto &arg : the_function->args())
        named_values_[arg.getName()] = &arg;

    if (Value *retval = body_->CodeGen()) {
        // Finish off the function.
        builder_.CreateRet(retval);

        // Validate the generated code, checking for consistency.
        verifyFunction(*the_function);

        return the_function;
    }

    // Error reading body, remove function.
    the_function->eraseFromParent();

    return nullptr;
}


