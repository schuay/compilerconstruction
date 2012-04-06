%{

#include <string.h>
#include <stdlib.h>

#include "common.hpp"

#define YYDEBUG 1

int yylex();
void yyerror(const char *p);
void process_funcdef(ExprAST *n);

int errcount = 0;

%}

%locations

%union {
    int sym;
    long val;
    ExprAST *n;
    SymList *syms;
    ExprList *exprs;
}

%start program

%token IDENT NUM END RETURN GOTO IF THEN VAR
%right '='
%nonassoc OPLESSEQ '#'
%left '+' AND NOT
%left '*'
%right DEREF UNARYMINUS

%%

program     :   /* empty */
            |   program funcdef ';'
                    { process_funcdef($<n>2); }
            ;
funcdef     :   IDENT '(' pars ')' stats END
                    { $<n>$ = new FunctionExprAST($<sym>1, $<syms>3, $<exprs>5); }
            ;
pars        :   /* empty */
                    { $<syms>$ = NULL; }
            |   IDENT
                    { $<syms>$ = SymList::push_back(NULL, $<sym>1); }
            |   IDENT ','  pars
                    { $<syms>$ = SymList::push_back($<syms>3, $<sym>1); }
            ;
stats       :   /* empty */
                    { $<exprs>$ = NULL; }
            |   stats singlestat ';'
                    { $<exprs>$ = ExprList::push_back($<exprs>1, $<n>2); }
            |   stats error ';' { yyerrok; }
                    { $<exprs>$ = $<exprs>1; }
            ;
singlestat  :   stat
                    { $<n>$ = new StatementExprAST($<n>1); }
            |   labels stat
                    { $<n>$ = new StatementExprAST($<syms>1, $<n>2); }
            ;
labels      :   IDENT ':'
                    { $<syms>$ = SymList::push_back(NULL, $<sym>1); }
            |   labels IDENT ':'
                    { $<syms>$ = SymList::push_back($<syms>1, $<sym>2); }
            ;
stat        :   RETURN expr
                    { $<n>$ = new UnaryExprAST(RETURN, $<n>2); }
            |   GOTO IDENT
                    { $<n>$ = new UnaryExprAST(GOTO, new SymbolExprAST($<sym>2, Label)); }
            |   IF expr THEN stats END
                    { $<n>$ = new IfExprAST($<n>2, $<exprs>4); }
            |   VAR IDENT '=' expr
                    { $<n>$ = new BinaryExprAST(VAR, new SymbolExprAST($<sym>2), $<n>4); }
            |   lexpr '=' expr
                    { $<n>$ = new BinaryExprAST('=', $<n>1, $<n>3); }
            |   term
            ;
lexpr       :   IDENT
                    { $<n>$ = new SymbolExprAST($<sym>1); }
            |   '*' unary
                    { $<n>$ = new UnaryExprAST(DEREF, $<n>2); }
            ;
termmul     :   term
            |   termmul '*' term
                    { $<n>$ = new BinaryExprAST('*', $<n>1, $<n>3); }
            ;
termplus    :   term
            |   termplus '+' term
                    { $<n>$ = new BinaryExprAST('+', $<n>1, $<n>3); }
            ;
termand     :   term
            |   termand AND term
                    { $<n>$ = new BinaryExprAST(AND, $<n>2, $<n>3); }
            ;
expr        :   unary
            |   termmul '*' term
                    { $<n>$ = new BinaryExprAST('*', $<n>1, $<n>3); }
            |   termplus '+' term
                    { $<n>$ = new BinaryExprAST('+', $<n>1, $<n>3); }
            |   termand AND term
                    { $<n>$ = new BinaryExprAST(AND, $<n>1, $<n>3); }
            |   term OPLESSEQ term
                    { $<n>$ = new BinaryExprAST(OPLESSEQ, $<n>1, $<n>3); }
            |   term '#' term
                    { $<n>$ = new BinaryExprAST('#', $<n>1, $<n>3); }
            ;
unary       :   NOT unary
                    { $<n>$ = new UnaryExprAST(NOT, $<n>2); }
            |   '-' unary
                    { $<n>$ = new UnaryExprAST(UNARYMINUS, $<n>2); }
            |   '*' unary
                    { $<n>$ = new UnaryExprAST(DEREF, $<n>2); }
            |   term
            ;
args        :   /* empty */
                    { $<exprs>$ = NULL; }
            |   expr
                    { $<exprs>$ = ExprList::push_back(NULL, $<n>1); }
            |   expr ',' args
                    { $<exprs>$ = ExprList::push_back($<exprs>3, $<n>1); }
            ;
term        :   '(' expr ')'
                    { $<n>$ = $<n>2; }
            |   NUM
                    { $<n>$ = new NumberExprAST($<val>1); }
            |   IDENT
                    { $<n>$ = new SymbolExprAST($<sym>1); }
            |   IDENT '(' args ')'
                    { $<n>$ = new CallExprAST($<sym>1, $<exprs>3); }
            ;

%%

void process_funcdef(ExprAST *n) {
    if (errcount > 0) {
        delete n;
        exit(ERR_SYNTAX);
    }
    n->collectDefinedSymbols();
    int err = n->checkSymbols(NULL);
    string s = n->toString(0);
    //printf("%s", s.c_str());

    Value *ret = n->codegen();
    if (ret == 0) {
        fprintf(stderr, "codegen() returned 0.\n");
    } else {
        theModule->dump();
    }

    delete n;
    
    if (err) {
        exit(ERR_SCOPE);
    }
}

void yyerror(const char *p) {
    fprintf(stderr, "ERROR line %d: %s\n", yylloc.first_line, p);
    errcount++;
}

int main(void) {
    yydebug = 0;

    initLLVM();

    yyparse();

    //printf("%s", syms.toString().c_str());
    delete theModule;

    if (errcount > 0) {
        return ERR_SYNTAX;
    }

    return 0;
}
