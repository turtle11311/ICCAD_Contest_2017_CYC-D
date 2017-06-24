%{
#include "InputSequenceGenerator.hpp"
#include <cstdio>
#include <iostream>
#include <list>
#include <string>
#include <map>
#include <vector>
extern int yylex(void);
extern int yylineno;
extern char* yytext;

using namespace SVParser;
using std::cout;
using std::endl;

int yyerror(SVParser::InputSequenceGenerator& FSM, const char* text) {
    fprintf(stderr, "%s is %s in line %d\n", text, yytext, yylineno);
    return 1;
}

// Variable
std::vector<std::string*> nameList;
std::map<std::string, unsigned int> paraTable;

int nowState = -1;
%}

%parse-param { SVParser::InputSequenceGenerator& FSM }

%union {
  std::string* string;
  SVParser::Range* range;
  SVParser::Pattern* pattern;
  unsigned int integer;
  SVParser::SignalChange sig_ch;
  SVParser::SignalEdge signal_edge;
}

%token k_MODULE                 "module"
%token k_ENDMODULE              "endmodule"
%token k_INPUT                  "input"
%token k_OUTPUT                 "output"
%token k_REG                    "reg"
%token k_PARAMETER              "parameter"
%token k_BEGIN                  "begin"
%token k_END                    "end"
%token k_POSEDGE                "posedge"
%token k_NEGEDGE                "negedge"
%token k_ALWAYS                 "always"
%token k_OR                     "or"
%token k_IF                     "if"
%token k_ELSE                   "else"
%token k_CASE                   "case"
%token k_CASEX                  "casex"
%token k_ENDCASE                "endcase"
%token k_ASSERT                 "assert"
%token k_PROPERTY               "property"
%token k_ROSE                   "$rose"
%token k_FELL                   "$fell"
%token k_DISPLAY                "$display"


%token STRING
%token <string> BIN_INTEGER
%token <string> DEC_INTEGER
%token <string> IDENTIFIER
%token <pattern> BIT_LABEL

%type <signal_edge> signal_change_identifier
%type <string> assertion_identifier
%type <string> port_identifier
%type <string> parameter_identifier
%type <integer> parameter
%type <integer> number
%type <range> range
%type <pattern> bit_pattern
%type <sig_ch> signal_change

%nonassoc LOWER_THAN_ELSE
%nonassoc "else"

%destructor { delete $$; } <string> <range> <pattern>

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
        | port_type range port_identifier_list
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
            $$ = new Range($2, $4);
        }
        | '[' number ']'
        {
            $$ = new Range($2);
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
            paraTable[*$1] = $3;
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
        : parameter { nowState = $1; } ':' casex_contruct
        ;

casex_contruct
        : "casex" '(' port_identifier ')' casex_list "endcase"
        ;

casex_list
        : casex_list casex
        | casex
        ;

casex
        : BIT_LABEL ':' "begin" port_identifier '=' parameter ';'
          port_identifier '=' bit_pattern ';' "end"
        {
            FSM.insesrtTransition(nowState, std::move(*$1), $6, std::move(*$10));
        }
        ;

non_blocking_assignment
        : port_identifier '<' '=' port_identifier
        ;

bit_pattern
        : BIN_INTEGER
        {
            $$ = new Pattern(*$1);
        }
        ;

assertion_rule
        : assertion_identifier ':' assertion_property_statement
        ;

assertion_property_statement
        : "assert" "property" '(' property_block ')' action_block
        ;

property_block
        : '@' '(' event_expression ')' signal_change '|' '-' '>' '#' '#' range signal_change
        {
            Assertion asrt;
            asrt.trigger = $5;
            asrt.event = $12;
            asrt.time = *$11;
            FSM.asrtList.push_back(asrt);
        }
        ;

signal_change
        : signal_change_identifier '(' port_identifier range ')'
        {
            $$.change = $1;
            $$.target = (*$3 != "in") ? TargetType::OUT : TargetType::IN;
            $$.index = $4->first;
        }
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

parameter
        : parameter_identifier { $$ = paraTable[*$1]; }
        ;

edge_identifier
        : "posedge"
        | "negedge"
        ;

signal_change_identifier
        : "$rose" { $$ = SignalEdge::ROSE; }
        | "$fell" { $$ = SignalEdge::FELL; }
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
