%start program
%token ID NUM END RETURN GOTO IF THEN VAR NOT AND
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
            |   term "=<" term
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
