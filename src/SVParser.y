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
%token k_BEGIN "begin"
%token k_END "end"
%token k_POSEDGE "posedge"
%token k_NEGEDGE "negedge"
%token k_ALWAYS "always"
%token k_OR "or"
%token k_IF "if"
%token k_ELSE "else"
%token k_CASE "case"
%token k_CASEX "casex"
%token k_ENDCASE "endcase"
%token k_ASSERT "assert"
%token k_PROPERTY "property"
%token k_ROSE "$rose"
%token k_FELL "$fell"
%token k_DISPLAY "$display"


%token STRING
%token <string> DEC_INTEGER
%token <string> IDENTIFIER
%token <string> BIT_LABEL

%type <string> assertion_identifier
%type <string> port_identifier
%type <string> parameter_identifier
%type <integer> number
%type <range> range

%nonassoc LOWER_THAN_ELSE
%nonassoc "else"

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
        | always_construct
        | assertion_rule
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

always_construct
        : "always" '@' '(' event_expression ')' sequential_block
        ;

event_expression
        : event_expression "or" event_expression
        | '*'
        | port_identifier
        | edge_identifier port_identifier
        ;

sequential_block
        : "begin" statement_list "end"
        | statement
        ;

statement_list
        : statement_list statement
        | statement
        ;

statement_or_epsilon
        : statement
        |
        ;

statement
        : if_construct
        | case_contruct
        | non_blocking_assignment ';'
        | display_string
        ;

if_construct
        : "if" '(' port_identifier ')' sequential_block %prec LOWER_THAN_ELSE
        | "if" '(' port_identifier ')' sequential_block "else" sequential_block
        ;

case_contruct
        : "case" '(' port_identifier ')' case_list "endcase"
        ;

case_list
        : case_list case
        | case
        ;

case
        : parameter_identifier ':' casex_contruct
        ;

casex_contruct
        : "casex" '(' port_identifier ')' casex_list "endcase"
        ;

casex_list
        : casex_list casex
        | casex
        ;

casex
        : BIT_LABEL ':' "begin" port_identifier '=' parameter_identifier ';'
          port_identifier '=' bit_pattern ';' "end"
        ;

non_blocking_assignment
        : port_identifier '<' '=' port_identifier
        ;

bit_pattern
        : DEC_INTEGER
        ;

assertion_rule
        : assertion_identifier ':' assertion_property_statement
        ;
    
assertion_property_statement
        : "assert" "property" '(' property_block ')' action_block
        ;

property_block
        : '@' '(' event_expression ')' signal_change '|' '-' '>' '#' '#' range signal_change
        ;

signal_change
        : signal_change_identifier '(' port_identifier range ')'
        ;

action_block
        : statement_or_epsilon "else" statement
        ;

display_string
        : "$display" '(' STRING ')' ';'

number
        : DEC_INTEGER
        {
            sscanf($1->c_str(), "%u", &$$);
        }
        ;

edge_identifier
        : "posedge"
        | "negedge"
        ;

signal_change_identifier
        : "$rose"
        | "$fell"
        ;

assertion_identifier
        : IDENTIFIER { $$ = $1; }
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