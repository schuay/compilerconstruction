letter_lower        [a-z]
letter_upper        [A-Z]
letter              ({letter_lower}|{letter_upper})
digit               [0-9]
hexdigit            ({digit}|[a-f]|[A-F])
identifier_start    ("_"|{letter})
identifier_body     ({identifier_start}|{digit})
identifier          {identifier_start}{identifier_body}*
hex_number          {digit}{hexdigit}+
dec_number          &{digit}+
comment_start       "(*"
comment_end         "*)"
whitespace          [ \t\n]
keyword             ("end"|"return"|"goto"|"if"|"then"|"var"|"not")
lexem               ("=<"|[;(),:=*\-+#])

    int errors = 0;

%x comment
%%

{comment_start}         BEGIN(comment);
<comment>{comment_end}  BEGIN(0);
<comment>.              ;
<comment><<EOF>>        fprintf(stderr, "Lexical error: open comment at end of file\n"); errors++; yyterminate();
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
