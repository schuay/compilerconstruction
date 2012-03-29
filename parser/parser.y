%{

    #include <stdio.h>
    #include <stdlib.h>

    #define YYDEBUG 1

    int yylex();
    int yyerror(const char *p);

%}

%union {
    double val;
    const char *name;
}

%start program
%token <name> ID
%token <val> NUM
%token END RETURN GOTO IF THEN VAR NOT AND OPLESSEQ
%right '='
%left '+'
%left '*'

%%

program     :   /* empty */
            |   program funcdef ';'
            ;
funcdef     :   ID '(' pars ')' stats END
            ;
pars        :   /* empty */
            |   ID
            |   ID ','  pars
            ;
stats       :   /* empty */
            |   stats singlestat ';'
            ;
singlestat  :   stat
            |   labeldef singlestat
            ;
labeldef    :   ID ':'
            ;
stat        :   RETURN expr
            |   GOTO ID
            |   IF expr THEN stats END
            |   VAR ID '=' expr
            |   lexpr '=' expr
            |   term
            ;
lexpr       :   ID
            |   '*' unary
            ;
termmul     :   term '*'
            |   termmul term '*'
            ;
termplus    :   term '+'
            |   termplus term '+'
            ;
termand     :   term AND
            |   termand term AND
            ;
expr        :   unary
            |   termmul term
            |   termplus term
            |   termand term
            |   term OPLESSEQ term
            |   term '#' term
            ;
unary       :   NOT unary
            |   '-' unary
            |   '*' unary
            |   term
            ;
args        :   /* empty */
            |   expr
            |   expr ',' args
            ;
term        :   '(' expr ')'
            |   NUM
            |   ID
            |   ID '(' args ')'
            ;

%%

int yyerror(const char *p) {
    fprintf(stderr, "%s\n", p);
    exit(2);
}

int main(void) {
    yydebug = 1;
    yyparse();
    return 0;
}
