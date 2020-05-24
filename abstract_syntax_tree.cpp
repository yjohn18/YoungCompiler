#include "abstract_syntax_tree.h"
#include "tools.h"
#include <vector>
#include <iostream>

llvm::LLVMContext kTheContext;
llvm::IRBuilder<> kBuilder(kTheContext);
std::unique_ptr<llvm::Module> kTheModule;
//std::map<std::string, llvm::Value *> kNamedValues;
std::map<std::string, llvm::AllocaInst *> kNamedValues;
std::unique_ptr<llvm::legacy::FunctionPassManager> kTheFpm;
std::unique_ptr<llvm::orc::KaleidoscopeJIT> kTheJit;
std::map<std::string, std::unique_ptr<PrototypeAst>> kFunctionProtos;


using namespace llvm;

Value *NumberExprAst::CodeGen() {
    return ConstantFP::get(kTheContext, APFloat(val_));
}

Value *VariableExprAst::CodeGen() {
    Value *V = kNamedValues[name_];

    if (!V) {
        std::cerr << "name " << name_ << std::endl;
        LogErrorV("Unknown variable name");
    }

    return kBuilder.CreateLoad(V, name_.c_str());
}

Value *BinaryExprAst::CodeGen() {
    // Special case for '=' because the LHS is a variable
    // rather than an expression.
    if (op_ == '=') {
        VariableExprAst *lhse = dynamic_cast<VariableExprAst*>(lhs_.get());
        if (!lhse)
            return LogErrorV("destination of '=' must be a variable");

        Value *val = rhs_->CodeGen();
        if (!val)
            return nullptr;

        // look up the name
        Value *v = kNamedValues[lhse->name()];
        if (!v)
            return LogErrorV("Unknown variable name");

        kBuilder.CreateStore(val, v);
        return val;
    }
    
    Value *l = lhs_->CodeGen();
    Value *r = rhs_->CodeGen();
    if (!l || !r)
        return nullptr;

    switch (op_) {
    case '+':
        return kBuilder.CreateFAdd(l, r, "addtmp");
    case '-':
        return kBuilder.CreateFSub(l, r, "subtmp");
    case '*':
        return kBuilder.CreateFMul(l, r, "multmp");
    case '<':
        l = kBuilder.CreateFCmpULT(l, r, "cmptmp");
        return kBuilder.CreateUIToFP(l, Type::getDoubleTy(kTheContext), "booltmp");
    default:
        return LogErrorV("invalid binary operator");
    }
}

Value *CallExprAst::CodeGen() {
    Function *callee_f = GetFunction(callee_);
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

    return kBuilder.CreateCall(callee_f, args_v, "calltmp");
}

Value *IfExprAst::CodeGen() {
    Value *cond_v = cond_->CodeGen();
    if (!cond_v)
        return nullptr;

    // Convert condition to a bool by comparing non-equal to 0.0.
    cond_v = kBuilder.CreateFCmpONE(cond_v, ConstantFP::get(kTheContext, APFloat(0.0)), "ifcond");

    Function *the_function = kBuilder.GetInsertBlock()->getParent();
    
    // Create blocks for the then and else cases.  Insert the 'then' block at the
    // end of the function.
    BasicBlock *then_bb = BasicBlock::Create(kTheContext, "then", the_function);
    BasicBlock *else_bb = BasicBlock::Create(kTheContext, "else");
    BasicBlock *merge_bb = BasicBlock::Create(kTheContext, "ifcont");

    kBuilder.CreateCondBr(cond_v, then_bb, else_bb);

    // Emit then value.
    kBuilder.SetInsertPoint(then_bb);

    Value *then_v = then_->CodeGen();
    if (!then_v)
        return nullptr;

    kBuilder.CreateBr(merge_bb);

    // Codegen of 'Then' can change the current block, update ThenBB for the PHI.
    then_bb = kBuilder.GetInsertBlock();

    // Emit else value.
    the_function->getBasicBlockList().push_back(else_bb);
    kBuilder.SetInsertPoint(else_bb);

    Value *else_v = else_->CodeGen();
    if (!else_v)
        return nullptr;

    kBuilder.CreateBr(merge_bb);
    // codegen of 'Else' can change the current block, update ElseBB for the PHI.
    else_bb = kBuilder.GetInsertBlock();

    // emit merge block.
    the_function->getBasicBlockList().push_back(merge_bb);
    kBuilder.SetInsertPoint(merge_bb);
    PHINode *pn = kBuilder.CreatePHI(Type::getDoubleTy(kTheContext), 2, "iftmp");

    pn->addIncoming(then_v, then_bb);
    pn->addIncoming(else_v, else_bb);

    return pn;
}

Value *WhileExprAst::CodeGen() {
    std::cerr << "start codegen while" << std::endl;
    //Value *cond_v = cond_->CodeGen();
    //if (!cond_v)
        //return nullptr;

    // Convert condition to a bool by comparing non-equal to 0.0.
    //cond_v = kBuilder.CreateFCmpONE(cond_v, ConstantFP::get(kTheContext, APFloat(0.0)), "whilecond");

    Function *the_function = kBuilder.GetInsertBlock()->getParent();
    
    // Create blocks for the then and else cases.  Insert the 'then' block at the
    // end of the function.
    BasicBlock *check_bb = BasicBlock::Create(kTheContext, "check", the_function);
    BasicBlock *loop_bb = BasicBlock::Create(kTheContext, "loop");
    BasicBlock *after_bb = BasicBlock::Create(kTheContext, "afterloop");

    //kBuilder.CreateCondBr(cond_v, loop_bb, after_bb);
    kBuilder.CreateBr(check_bb);
    
    // Emit then value.
    kBuilder.SetInsertPoint(check_bb);

    Value *cond_v = cond_->CodeGen();
    if (!cond_v)
        return nullptr;
    
    cond_v = kBuilder.CreateFCmpONE(cond_v, ConstantFP::get(kTheContext, APFloat(0.0)), "whilecond");
    kBuilder.CreateCondBr(cond_v, loop_bb, after_bb);

    the_function->getBasicBlockList().push_back(loop_bb);
    kBuilder.SetInsertPoint(loop_bb);
    if (!body_->CodeGen())
        return nullptr;

    kBuilder.CreateBr(check_bb);

    // Emit else value.
    the_function->getBasicBlockList().push_back(after_bb);
    kBuilder.SetInsertPoint(after_bb);

    std::cerr << "finish codegen while" << std::endl;
    return Constant::getNullValue(Type::getDoubleTy(kTheContext));
}

Value *IntExprAst::CodeGen() {
    std::vector<AllocaInst *> old_bindings;

    Function *the_function = kBuilder.GetInsertBlock()->getParent();

    // Register all variables and emit their initializer.
    for (unsigned i = 0, e = var_names_.size(); i != e; ++i) {
        const std::string &var_name = var_names_[i].first;
        ExprAst *init = var_names_[i].second.get();

        Value *init_val;
        if (init) {
            init_val = init->CodeGen();
            if (!init_val)
                return nullptr;
        } else {
            init_val = ConstantFP::get(kTheContext, APFloat(0.0));
        }

        AllocaInst *alloca = CreateEntryBlockAlloca(the_function, var_name);
        kBuilder.CreateStore(init_val, alloca);

        // Remember the old variable binding 
        // so that we can restore the binding when we unrecurse.
        old_bindings.push_back(kNamedValues[var_name]);

        // Remember this binding.
        kNamedValues[var_name] = alloca;
    }

    // Codegen the body.
    Value *body_val = body_->CodeGen();
    if (!body_val)
        return nullptr;

    // Pop all our variables from scope.
    for (unsigned i = 0, e = var_names_.size(); i != e; ++i)
        kNamedValues[var_names_[i].first] = old_bindings[i];

    return Constant::getNullValue(Type::getDoubleTy(kTheContext));
}

Function *PrototypeAst::CodeGen() {
    std::vector<Type *> doubles(args_.size(), Type::getDoubleTy(kTheContext));

    FunctionType *ft = FunctionType::get(Type::getDoubleTy(kTheContext), doubles, false);

    Function *f = Function::Create(ft, Function::ExternalLinkage, name_, kTheModule.get());

    // Set names for all arguments.
    unsigned idx = 0;
    for (auto &arg : f->args())
        arg.setName(args_[idx++]);

    return f;
}

Function *FunctionAst::CodeGen() {
    // First check for an existing function from a previous "extern" declaration.
    auto &p = *proto_;
    kFunctionProtos[proto_->name()] = std::move(proto_);
    Function *the_function = GetFunction(p.name());

    if (!the_function)
        the_function = proto_->CodeGen();

    if (!the_function)
        return nullptr;

    if (!the_function->empty())
        return (Function*)LogErrorV("Function cannot be redefined.");

    // Create a new basic block to start insertion into.
    BasicBlock *bb  = BasicBlock::Create(kTheContext, "entry", the_function);
    kBuilder.SetInsertPoint(bb);

    // Record the function arguments in the NamedValues map.
    kNamedValues.clear();
    for (auto &arg : the_function->args()) {
        // Create an alloca for this argument.
        AllocaInst *alloca = CreateEntryBlockAlloca(the_function, arg.getName());

        // Store the initial value into the alloca.
        kBuilder.CreateStore(&arg, alloca);

        // Add  arguments to variable symbol table.
        kNamedValues[arg.getName()] = alloca;
    }

    if (Value *retval = body_->CodeGen()) {
        // Finish off the function.
        kBuilder.CreateRet(retval);

        // Validate the generated code, checking for consistency.
        verifyFunction(*the_function);
        
        //kTheFpm->run(*the_function);

        return the_function;
    }

    // Error reading body, remove function.
    the_function->eraseFromParent();

    return nullptr;
}


