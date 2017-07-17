%{
#include <string>
#include "InputSequenceGenerator.hpp"
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
"begin"                 return k_BEGIN;
"end"                   return k_END;
"posedge"               return k_POSEDGE;
"negedge"               return k_NEGEDGE;
"always"                return k_ALWAYS;
"or"                    return k_OR;
"if"                    return k_IF;
"else"                  return k_ELSE;
"case"                  return k_CASE;
"casex"                 return k_CASEX;
"endcase"               return k_ENDCASE;
"assert"                return k_ASSERT;
"property"              return k_PROPERTY;
"$rose"                 return k_ROSE;
"$fell"                 return k_FELL;
"$display"              return k_DISPLAY;


{D_INT}'[Bb]{B_INT}     { parse_integer(); make_string(tmps); return BIN_INTEGER; }
{D_INT}'[Dd]{D_INT}     { parse_integer(); make_string(tmps); return DEC_INTEGER; }
{D_INT}'[Bb]{XINT}      { parse_integer(); make_string(tmps); return BIT_LABEL; }
{D_INT}                 { make_string(yytext); return DEC_INTEGER; }
{IDENTIFY}              { make_string(yytext); return IDENTIFIER; }
\".*\"                  return STRING;
.                       return yytext[0];

%%
