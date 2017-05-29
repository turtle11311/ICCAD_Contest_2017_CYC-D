%{
#include <string>
#include "SVParser.hpp"
#include "SVParser.tab.hpp"

#define make_string(arg) yylval.string = new std::string((arg));
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


{D_INT}                 { make_string(yytext); return DEC_INTEGER; }
{IDENTIFY}              { make_string(yytext); return IDENTIFIER; }
.                       return yytext[0];

%%