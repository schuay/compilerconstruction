%{

    #include <stdio.h>
    #include <stdlib.h>

    #define YYDEBUG 1

    int yylex();
    void yyerror(const char *p);

    int errcount = 0;
%}

%union {
    double val;
    const char *name;
}

%locations

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
            |   stats error ';' { yyerrok; }
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

void yyerror(const char *p) {
    fprintf(stderr, "ERROR line %d: %s\n", yylloc.first_line, p);
    errcount++;
}

int main(void) {
    yydebug = 0;
    yyparse();
    if (errcount > 0) {
        return 2;
    }
    return 0;
}
