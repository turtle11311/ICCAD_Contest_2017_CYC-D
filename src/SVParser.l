%{
#include <string>
#include "SVParser.tab.hpp"
%}

IDENTIFY        [[:alpha:]_][[:alnum:]_]*

INT           [[:xdigit:]]+
XINT          [01xX]+

%option yylineno noyywrap
%%

[[:blank:]\n]+          ;

module                  return k_MODULE;
endmodule               return k_ENDMODULE;

{IDENTIFY}              { yylval.string = new std::string(yytext); return IDENTIFIER; }
.                       return yytext[0];

%%