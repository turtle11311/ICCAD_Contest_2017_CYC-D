%{
#include "SVParser.hpp"
#include <cstdio>
#include <iostream>
#include <string>
#include <map>
#include <vector>
extern int yylex(void);
extern int yylineno;
extern char* yytext;
int yyerror(const char* text) {
    fprintf(stderr, "%s is %s in line %d\n", text, yytext, yylineno);
    return 1;
}
using namespace SVParser;
using std::cout;
using std::endl;

// Variable
std::map<std::string, Pattern<>* > varMap;
std::vector<std::string*> nameList;
std::map<std::string, unsigned int> parameterTable;

%}

%union {
  std::string* string;
  SVParser::range* range;
  unsigned int integer;
}

%token k_MODULE "module"
%token k_ENDMODULE "endmodule"
%token k_INPUT "input"
%token k_OUTPUT "output"
%token k_REG "reg"
%token k_PARAMETER "parameter"

%token <string> DEC_INTEGER
%token <string> IDENTIFIER

%type <string> port_identifier
%type <string> parameter_identifier
%type <integer> number
%type <range> range

%destructor { delete $$; } <string> <range>

%start source_text

%%

source_text
        : module_header module_item_list "endmodule"
        ;

module_header
        : "module" module_identifier '(' port_identifier_list ')' ';'
        ;

module_item_list
        : module_item_list module_item
        | module_item
        ;

module_item
        : port_declaration ';'
        | parameter_declaration ';'
        ;

port_declaration
        : port_type port_identifier_list
        {
            for (auto it = nameList.begin(); it != nameList.end(); ++it) {
                if (varMap.find(**it) == varMap.end()) {
                    varMap[**it] = new Pattern<>(1);
                }
            }
        }
        | port_type range port_identifier_list
        {
            for (auto it = nameList.begin(); it != nameList.end(); ++it) {
                if (varMap.find(**it) == varMap.end()) {
                    varMap[**it] = new Pattern<>($2->length());
                }
            }
        }
        ;

port_type
        : "input"
        | "output"
        | "reg"
        ;

port_identifier_list
        : port_identifier_list ',' port_identifier
        {
            nameList.push_back($3);
        }
        | port_identifier
        {
            nameList.clear();
            nameList.push_back($1);
        }
        ;

range
        : '[' number ':' number ']'
        {
            $$ = new range($2, $4);
        }
        | '[' number ']'
        {
            $$ = new range($2);
        }
        ;

parameter_declaration
        : "parameter" parameter_assignment_list
        ;

parameter_assignment_list
        : parameter_assignment_list ',' parameter_assignment
        | parameter_assignment
        ;

parameter_assignment
        : parameter_identifier '=' number
        {
            parameterTable[*$1] = $3;
        }
        ;

number
        : DEC_INTEGER
        {
            sscanf($1->c_str(), "%u", &$$);
        }
        ;

port_identifier
        : IDENTIFIER { $$ = $1; }
        ;

parameter_identifier
        : IDENTIFIER { $$ = $1; }
        ;

module_identifier
        : IDENTIFIER
        ;
%%