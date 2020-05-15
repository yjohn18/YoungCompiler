#include "parser.h"
#include "tools.h"
#include <cctype>
#include <utility>
#include <iostream>
#include <vector>
#include <cstdint>
#include <cassert>
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"

extern llvm::LLVMContext kTheContext;
extern llvm::IRBuilder<> kBuilder;
extern std::unique_ptr<llvm::Module> kTheModule;
extern std::map<std::string, llvm::Value *> kNamedValues;
extern std::unique_ptr<llvm::legacy::FunctionPassManager> kTheFpm;
extern std::unique_ptr<llvm::orc::KaleidoscopeJIT> kTheJit;
extern std::map<std::string, std::unique_ptr<PrototypeAst>> kFunctionProtos;


using namespace std;

int Parser::GetNextToken() {
    return cur_tok_ = lexer_.GetTok();
}

int Parser::GetTokPrecedence() {
    if (!isascii(cur_tok_))
        return -1;

    int tok_prec = bin_op_precedence_[cur_tok_];
    if (tok_prec <= 0)
        return -1;
    return tok_prec;
}

unique_ptr<ExprAst> Parser::ParseNumberExpr() {
    auto result = make_unique<NumberExprAst>(lexer_.num_val());
    GetNextToken();
    return move(result);
}

unique_ptr<ExprAst> Parser::ParseParenExpr() {
    GetNextToken(); // eat '('
    auto expr = ParseExpression();
    if (!expr)
        return nullptr;

    if (cur_tok_ != ')')
        return LogError("expected ')'");
    GetNextToken(); // eat ')'
    return expr;
}

unique_ptr<ExprAst> Parser::ParseIdentifierExpr() {
    string id_name = lexer_.identifier_str();

    GetNextToken();

    if (cur_tok_ != '(')
        // simple variable reference
        // construct a VariableExpreAst
        return make_unique<VariableExprAst>(id_name);

    // cur_tok_ == '(', which means a function call
    // construct a CallExprAst
    GetNextToken(); // eat '('
    vector<unique_ptr<ExprAst>> args;
    if (cur_tok_ != ')') {
        while (true) {
            if (auto arg = ParseExpression())
                args.push_back(move(arg));
            else
                return nullptr;

            if (cur_tok_ == ')')
                break;

            if (cur_tok_ != ',')
                return LogError("Expected ')' or ',' in argument list");
            GetNextToken();
        }
    }

    GetNextToken(); // eat ')'

    return make_unique<CallExprAst>(id_name, move(args));
}

unique_ptr<ExprAst> Parser::ParsePrimary() {
    switch (cur_tok_) {
        case kTokIdentifier:
            return ParseIdentifierExpr();
        case kTokNumber:
            return ParseNumberExpr();
        case '(':
            return ParseParenExpr();
        case kTokIf:
            return ParseIfExpr();
        case kTokWhile:
            return ParseWhileExpr();
        default:
            cerr << "cut_tok_ " << cur_tok_ << endl;
            return LogError("unknown token when expecting an expression");
    }
}

unique_ptr<ExprAst> Parser::ParseBinOpRhs(int expr_prec, unique_ptr<ExprAst> lhs) {
    // If this is a binop, find its precedence.
    while (true) {
        int tok_prec = GetTokPrecedence();

        // If this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done.
        if (tok_prec < expr_prec)
            return lhs;

        // Okay, we know this is a binop. 
        int bin_op = cur_tok_;
        GetNextToken();

        // Parse the primary expression after the binary operator.
        auto rhs = ParsePrimary();
        if (!rhs)
            return nullptr;

        // If BinOp binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int next_prec = GetTokPrecedence();
        if (tok_prec < next_prec) {
            rhs = ParseBinOpRhs(tok_prec + 1, move(rhs));
            if (!rhs)
                return nullptr;
        }

        // Merge LHS/RHS.
        lhs = make_unique<BinaryExprAst>(bin_op, move(lhs), move(rhs));
    }
}

unique_ptr<ExprAst> Parser::ParseExpression() {
    auto lhs = ParsePrimary();
    if (!lhs)
        return nullptr;

    return ParseBinOpRhs(0, move(lhs));
}

unique_ptr<PrototypeAst> Parser::ParsePrototype() {
    if (cur_tok_ != kTokIdentifier)
        return LogErrorP("Expected function name in prototype");

    string fn_name = lexer_.identifier_str();
    GetNextToken();

    if (cur_tok_ != '(')
        return LogErrorP("Expected '(' in prototype");

    vector<string> arg_names;
    while (GetNextToken() == kTokIdentifier)
        arg_names.push_back(lexer_.identifier_str());
    if (cur_tok_ != ')')
        return LogErrorP("Expected ')' in prototype");

    GetNextToken();

    return make_unique<PrototypeAst>(fn_name, move(arg_names));
}

unique_ptr<FunctionAst> Parser::ParseDefinition() {
    GetNextToken();
    auto proto = ParsePrototype();
    if (!proto)
        return nullptr;

    if (auto expr = ParseExpression())
        return make_unique<FunctionAst>(move(proto), move(expr));
    return nullptr;
}


unique_ptr<FunctionAst> Parser::ParseTopLevelExpr() {
    if (auto expr = ParseExpression()) {
        auto proto = make_unique<PrototypeAst>("__anon_expr", vector<string>());
        return make_unique<FunctionAst>(move(proto), move(expr));
    }
    return nullptr;
}

unique_ptr<PrototypeAst> Parser::ParseExtern() {
    GetNextToken();
    return ParsePrototype();
}

unique_ptr<ExprAst> Parser::ParseIfExpr() {
    GetNextToken(); // eat 'if'

    if (cur_tok_ != '(')
        return LogError("expected '('");
    GetNextToken(); // eat '('

    // condition
    auto cond = ParseExpression();
    if (!cond)
        return nullptr;

    if (cur_tok_ != ')')
        return LogError("expected ')'");
    GetNextToken(); // ')'

    auto then_expr = ParseExpression();
    if (!then_expr)
        return nullptr;

    if (cur_tok_ != kTokElse) {
        // if - then expression
        cerr << "cur_tok_ " << cur_tok_ << endl;
        return LogError("expected else");
    }

    GetNextToken();

    auto else_expr = ParseExpression();
    if (!else_expr)
        return nullptr;
    return make_unique<IfExprAst>(move(cond), move(then_expr), move(else_expr));
}

unique_ptr<ExprAst> Parser::ParseWhileExpr() {
    GetNextToken(); // eat 'while'

    if (cur_tok_ != '(')
        return LogError("expected '(' after while");
    GetNextToken(); // eat '('

    // condition
    auto cond = ParseExpression();
    if (!cond)
        return nullptr;

    if (cur_tok_ != ')')
        return LogError("expected ')'");
    GetNextToken(); // eat ')'

    auto body = ParseExpression();
    if (!body)
        return nullptr;

    cerr << "parse while expression done" << endl;
    return make_unique<WhileExprAst>(move(cond), move(body));
}

void Parser::HandleDefinition() {
    if (auto fn_ast = ParseDefinition()) {
        if (auto *fn_ir = fn_ast->CodeGen()) {
            cerr << "Read function definition: ";
            fn_ir->print(llvm::errs());
            cerr << endl;
            kTheJit->addModule(std::move(kTheModule));
            InitializeModuleAndPassManager();
        }
    } else {
        GetNextToken();
    }
}

void Parser::HandleExtern() {
    if (auto proto_ast = ParseExtern()) {
        if (auto *fn_ir = proto_ast->CodeGen()) {
            cerr << "Read extern: ";
            fn_ir->print(llvm::errs());
            cerr << endl;
            kFunctionProtos[proto_ast->name()] = move(proto_ast);
        }
    } else {
        GetNextToken();
    }
}

void Parser::HandleTopLevelExpression() {
    if (auto fn_ast = ParseTopLevelExpr()) {
        if (auto *fn_ir = fn_ast->CodeGen()) {
            
            auto h = kTheJit->addModule(move(kTheModule));
            InitializeModuleAndPassManager();

            auto expr_symbol = kTheJit->findSymbol("__anon_expr");
            assert(expr_symbol && "Function not found");

            double (*fp)() = (double (*)())(intptr_t)llvm::cantFail(expr_symbol.getAddress());
            cerr << "Evaluated to " << fp() << endl;

            kTheJit->removeModule(h);
        }
    } else {
        GetNextToken();
    }
}

void Parser::MainLoop() {
    while (true) {
        switch (cur_tok_) {
            case kTokEof:
                return;
            case ';' :
                GetNextToken();
                break;
            case kTokDef:
                HandleDefinition();
                break;
            case kTokExtern:
                HandleExtern();
                break;
            default:
                HandleTopLevelExpression();
                break;
        }
    }
}

Parser::Parser(string file_path) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    
    bin_op_precedence_['<'] = 10;
    bin_op_precedence_['+'] = 20;
    bin_op_precedence_['-'] = 20;
    bin_op_precedence_['*'] = 40;
    
    lexer_.SetFilePath(file_path);
    if (!lexer_.IsFileOpen())
        cerr << "fail to open source file " << file_path << endl;
    
    GetNextToken();

    kTheJit = make_unique<llvm::orc::KaleidoscopeJIT>();

    InitializeModuleAndPassManager();
}





