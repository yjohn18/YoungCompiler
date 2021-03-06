#ifndef PARSER_H
#define PARSER_H

#include <map>
#include <memory>
#include "abstract_syntax_tree.h"
#include "lexer.h"
#include <llvm-9/llvm/ADT/APFloat.h>
#include <llvm-9/llvm/ADT/STLExtras.h>
#include <llvm-9/llvm/IR/BasicBlock.h>
#include <llvm-9/llvm/IR/Constants.h>
#include <llvm-9/llvm/IR/DerivedTypes.h>
#include <llvm-9/llvm/IR/Function.h>
#include <llvm-9/llvm/IR/IRBuilder.h>
#include <llvm-9/llvm/IR/LLVMContext.h>
#include <llvm-9/llvm/IR/Module.h>
#include <llvm-9/llvm/IR/Type.h>
#include <llvm-9/llvm/IR/Verifier.h>


class Parser {
protected:
    Lexer lexer_;
    int cur_tok_;

    /* LLVM objects */
//    llvm::LLVMContext the_context_;
//    llvm::IRBuilder<> builder_;
//    std::unique_ptr<llvm::Module> the_module_;
//    std::map<std::string, llvm::Value *> named_values_;

    // BinopPrecedence - This holds the precedence for each binary operator that
    // is defined.
    std::map<char, int> bin_op_precedence_;

    int GetNextToken();

    // GetTokPrecedence - Get the precedence of the pending binary operator token.
    int GetTokPrecedence();

    // numberexpr ::= number
    std::unique_ptr<ExprAst> ParseNumberExpr();

    // parenexpr ::= '(' expression ')'
    std::unique_ptr<ExprAst> ParseParenExpr();

    // identifierexpr
    //   ::= identifier
    //   ::= identifier '(' expression* ')'
    //   the second is function call
    std::unique_ptr<ExprAst> ParseIdentifierExpr();

    // primary
    //   ::= identifierexpr
    //   ::= numberexpr
    //   ::= parenexpr
    std::unique_ptr<ExprAst> ParsePrimary();
    
    // binoprhs (binary oprator right hand side)
    //   ::= ('+' primary)*
    std::unique_ptr<ExprAst> ParseBinOpRhs(int expr_prec, 
            std::unique_ptr<ExprAst> lhs);

    // statement
    //   ::= expression
    //   ::= '{' expression* '}'


    // expression
    //   ::= primary binoprhs
    std::unique_ptr<ExprAst> ParseExpression();

    // prototype
    // ::= id '(' id* ')'
    std::unique_ptr<PrototypeAst> ParsePrototype();

    // definition ::= 'def' prototype '{' expression '}'
    std::unique_ptr<FunctionAst> ParseDefinition();

    // toplevelexpr ::= expression
    std::unique_ptr<FunctionAst> ParseTopLevelExpr();

    // external ::= 'extern' prototype
    std::unique_ptr<PrototypeAst> ParseExtern();
    
    // ifexpr ::= 'if' '(' expression ')' expression 'else' expression
    std::unique_ptr<StatAst> ParseIfStat();
    
    // whileexpr ::= 'while' '(' expression ')' expression
    std::unique_ptr<StatAst> ParseWhileStat();

    std::unique_ptr<StatAst> ParseReturnStat();

    std::unique_ptr<StatAst> ParseAssignmentStat();

    // intexpr ::= 'int' identifier ('=' expression)?
    // (',' identifier ('=' expression)?)* 'in' expression
    std::unique_ptr<CompoundStatAst> ParseCompoundStat();

    std::unique_ptr<StatListAst> ParseStatList();


    /* Top-Level parsing */
    void HandleDefinition();
    void HandleExtern();
    void HandleTopLevelExpression();
public:
    Parser(std::string);
//    ~Parser();

    // main loop
    // top ::= definition | external | expression | ';'
    void MainLoop();
};



#endif
