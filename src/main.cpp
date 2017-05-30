#include "SVParser.hpp"
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <list>
#include <map>

using namespace SVParser;
using std::cout;
using std::endl;

extern int yyparse(void);

extern std::map<std::string, Pattern<>* > varMap;
extern std::map<std::string, unsigned int> parameterTable;
extern std::map< int, std::list<Transition> > fsm;
extern std::list<Assertion> asrtList;

int main (int argc, const char* argv[])
{
    yyparse();
    
    for (auto it = varMap.begin(); it != varMap.end(); ++it) {
        delete it->second;
    }
    return EXIT_SUCCESS;
}