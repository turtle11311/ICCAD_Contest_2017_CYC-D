%{
#include <string>
#include "SVParser.hpp"
#include "SVParser.tab.hpp"

char tmps[100];
char tmpc;
int tmpi;

#define make_string(arg) yylval.string = new std::string((arg));
#define parse_integer() sscanf(yytext, "%d'%c%s", &tmpi , &tmpc, tmps );
%}

IDENTIFY        [[:alpha:]_][[:alnum:]_]*

B_INT           [01]+
O_INT           [0-8]+
D_INT           [[:digit:]]+
H_INT           [[:xdigit:]]+
XINT            [01xX]+

%option yylineno noyywrap
%%

[[:blank:]\n]+          ;

"module"                return k_MODULE;
"endmodule"             return k_ENDMODULE;
"input"                 return k_INPUT;
"output"                return k_OUTPUT;
"reg"                   return k_REG;
"parameter"             return k_PARAMETER;

{D_INT}'[Dd]{D_INT}     { parse_integer(); make_string(tmps); return DEC_INTEGER; }
{D_INT}                 { make_string(yytext); return DEC_INTEGER; }
{IDENTIFY}              { make_string(yytext); return IDENTIFIER; }
.                       return yytext[0];

%%