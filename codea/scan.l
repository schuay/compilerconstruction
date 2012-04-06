%{
    #include "common.hpp"
    #include "gram.tab.hpp"

    #define YY_USER_ACTION yylloc.first_line = yylloc.last_line = yylineno;

    #ifdef __GNUC__
    static void yyunput(int c, register char * yy_bp) __attribute__((unused));
    #endif
%}

%option yylineno

letter              [a-zA-Z]
digit               [0-9]
hexdigit            ({digit}|[a-fA-F])
identifier_start    ("_"|{letter})
identifier_body     ({identifier_start}|{digit})
identifier          {identifier_start}{identifier_body}*
 /* The first digit is already part of the number. */
hex_number          {digit}{hexdigit}*
 /* A single & is invalid syntax. */
dec_number          &{digit}+
comment_start       "(*"
comment_end         "*)"
 /* Comments are the shortest section between a starting (* and a closing *).
    If not closed, the (* is interpreted as 2 lexemes ( and *.
    Comments can be multiline. A simpler regexp with nongreedy matching could be:
    {comment_start}(.|\n)*?{comment_end}  */
comment             {comment_start}(([^*])|([*][^\)]))*{comment_end}
whitespace          [ \t\n]
lexem               [;(),:=*\-+#]

%%

end                     return END;
return                  return RETURN;
goto                    return GOTO;
if                      return IF;
then                    return THEN;
var                     return VAR;
not                     return NOT;
and                     return AND;
=<                      return OPLESSEQ;
{lexem}                 return yytext[0];
{hex_number}            yylval.val = strtol(yytext, NULL, 16); return NUM;
{dec_number}            yylval.val = strtol(yytext + 1, NULL, 10); return NUM;
{identifier}            yylval.sym = syms.insert(yytext); return IDENT;
{whitespace}+           ;
{comment}               ;
.                       { fprintf(stderr, "ERROR line %d: '%s'\n", yylloc.first_line, yytext); exit(1); }
