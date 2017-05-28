%{
#include <iostream>
#include <string>
extern int yylex(void);
extern int yylineno;
extern char* yytext;
int yyerror(const char* text) {
    fprintf(stderr, "%s is %s in line %d\n", text, yytext, yylineno);
    return 1;
}
%}

%union {
  std::string* string;
}

%token k_MODULE "module"
%token k_ENDMODULE "endmodule"

%token <string> IDENTIFIER

%destructor { delete $$; } <string>

%start source_text

%%

source_text
        : module_header module_item_list "endmodule"
        ;

module_header
        : "module" module_identifier '(' port_identifier_list ')' ';'
        ;

module_item_list
        :
        ;

port_identifier_list
        : port_identifier_list ',' port_identifier
        | port_identifier
        ;

port_identifier
        : IDENTIFIER
        ;

module_identifier
        : IDENTIFIER
        ;
%%