#include <iostream>
#include <cstdlib>

extern int yyparse(void);

int main (int argc, const char* argv[])
{
    yyparse();
    return EXIT_SUCCESS;
}