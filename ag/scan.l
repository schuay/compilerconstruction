%{
    #include "gram.tab.h"
    #define YY_USER_ACTION yylloc.first_line = yylloc.last_line = yylineno;
%}

%option yylineno

letter_lower        [a-z]
letter_upper        [A-Z]
letter              ({letter_lower}|{letter_upper})
digit               [0-9]
hexdigit            ({digit}|[a-f]|[A-F])
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

"end"                   return END;
"return"                return RETURN;
"goto"                  return GOTO;
"if"                    return IF;
"then"                  return THEN;
"var"                   return VAR;
"not"                   return NOT;
"and"                   return AND;
"=<"                    return OPLESSEQ;
{lexem}                 return yytext[0];
{hex_number}            return NUM;
{dec_number}            return NUM;
{identifier}            return ID;
{whitespace}+           ;
{comment}               ;
.                       { fprintf(stderr, "ERROR line %d: syntax error '%s'\n",
                                yylloc.first_line, yytext); exit(1); }
