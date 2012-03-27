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
keyword             ("end"|"return"|"goto"|"if"|"then"|"var"|"not"|"and")
lexem               ("=<"|[;(),:=*\-+#])

    int errors = 0;

%%

{comment}               ;
{keyword}               |
{lexem}                 printf("%s\n", yytext);
{hex_number}            printf("num %d\n", strtol(yytext, NULL, 16));
{dec_number}            printf("num %d\n", strtol(yytext + 1, NULL, 10));
{identifier}            printf("ident %s\n", yytext);
{whitespace}            ;
.                       fprintf(stderr, "Lexical error: %s\n", yytext); errors++;

%%

int main(void) {
    int ret = 0;
    yylex();
    if (errors) {
        fprintf(stderr, "%d error(s) occurred.\n", errors);
        ret = 1;
    }
    return ret;
}
