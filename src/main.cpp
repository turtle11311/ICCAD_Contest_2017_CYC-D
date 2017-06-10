#include "SVParser.hpp"
#include <cstdlib>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <vector>

using namespace SVParser;
using std::cout;
using std::endl;

extern int yyparse(void);

extern std::map< std::string, Pattern* > varMap;
extern std::list< Assertion > asrtList;
extern FiniteStateMachine FSM;

int main(int argc, const char* argv[])
{
    yyparse();

    for (auto it = varMap.begin(); it != varMap.end(); ++it) {
        delete it->second;
    }
    return EXIT_SUCCESS;
}