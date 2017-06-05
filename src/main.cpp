#include "SVParser.hpp"
#include <cstdlib>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <ctime>

using namespace SVParser;
using std::cout;
using std::endl;

extern int yyparse(void);

extern std::map< std::string, Pattern<>* > varMap;
extern std::map< std::string, unsigned int > parameterTable;
extern std::map< int, std::list< Transition > > fsm;
extern std::list< Assertion > asrtList;

int size;
int* state = new int;
std::vector<Pattern<>> inputSequence;
std::vector<unsigned int> rstRecord;

void simulator();
void activator(Assertion&);
bool outActivate(unsigned int, unsigned int);
bool recursionChecker( int );
void terminator(Assertion&);
void reset();
void preOperationForSimulator();
std::pair<bool,unsigned int> find(unsigned short*);
void printInputSequence();
void printOutputSequence();
int main(int argc, const char* argv[])
{
    srand(time(0));
    yyparse();
    simulator();
    for (auto it = varMap.begin(); it != varMap.end(); ++it) {
        delete it->second;
    }
    delete state;
    return EXIT_SUCCESS;
}

void simulator(){
    const int ptnSize = size = fsm[0].front().pattern.size();
    inputSequence.push_back(Pattern<>(ptnSize));
    preOperationForSimulator();
    int count = 0;
    for ( auto it = asrtList.begin() ; it != asrtList.end() ; ++it ){
        cout << "Assertion: " << ++count << endl;
        activator(*it);
        reset();
    }
    printInputSequence();
}

void activator( Assertion& asrt ){
    unsigned int triggerFlag = asrt.trigger.change;
    bool activated = false;
    unsigned int index = 0;
    cout << "initial out: " << *varMap["out"] << endl;

    // false = in : true = out | index
    std::pair<bool,unsigned int> io = find(asrt.trigger.target);
    if ( io.first ){
        if ( triggerFlag ){
            if ( (*varMap["out"])[index] == !triggerFlag ){
                while (!outActivate(io.second, triggerFlag)) {

                }
            } else {

            }
        } else {

        }
    } else{

    }
}

bool outActivate(unsigned int index , unsigned int triggerFlag ){

    cout << "Out Activate start.\n";
    cout << "Current state: " << *state << endl;
    for ( auto it = fsm[*state].begin() ; it != fsm[*state].end() ; ++it){
        //first meet
        if ( it->out[index] == triggerFlag && !it->traversed ){
            cout << "Assertion has been activated!\n";
            it->traversed = true;
            *state = it->nstate;
            *varMap["out"] = it->out;
            inputSequence.push_back(it->defaultPattern());
            printOutputSequence();
            cout << "Next state: " << *state << endl;
            return true;
        } else if ( it->out[index] == triggerFlag ){
            cout << "Assertion has been activated!\n";
            unsigned int idx = rand() % fsm[*state].size();
            unsigned int i = 0;
            for ( auto rand_it = fsm[*state].begin(); rand_it != fsm[*state].end() ; ++rand_it, ++i ){
                if ( i == idx  ){
                    *state = rand_it->nstate;
                    *varMap["out"] = rand_it->out;
                    inputSequence.push_back(rand_it->defaultPattern());
                    printOutputSequence();
                    cout << "Next state: " << *state << endl;
                    return true;
                }
            }
        }
    }
    for ( auto it = fsm[*state].begin() ; it != fsm[*state].end() ; ++it){
        if ( *state != it->nstate && !it->traversed ){
            it->traversed = true;
            *state = it->nstate;
            *varMap["out"] = it->out;
            inputSequence.push_back(it->defaultPattern());
            printOutputSequence();
            cout << "Next state: " << *state << endl;
            break;
        } else if ( *state != it->nstate ){
            unsigned int idx = rand() % fsm[*state].size();
            unsigned int i = 0;
            for ( auto rand_it = fsm[*state].begin(); rand_it != fsm[*state].end() ; ++rand_it, ++i ){
                if ( i == idx  ){
                    *state = rand_it->nstate;
                    *varMap["out"] = rand_it->out;
                    inputSequence.push_back(rand_it->defaultPattern());
                    printOutputSequence();
                    cout << "Next state: " << *state << endl;
                    break;
                }
            }
            break;
        }
    }
    return false;
}

void reset(){
    cout << "Reset." << endl;
    rstRecord.push_back(inputSequence.size());
    *state = 0;
    for ( auto it = fsm[*state].begin() ; it != fsm[*state].end() ; ++it ){
        if ( !it->traversed ){
            it->traversed = true;
            *state = it->nstate;
            *varMap["out"] = it->out;
            inputSequence.push_back(it->defaultPattern());
            printOutputSequence();
            cout << "Next state: " << *state << endl;
            return;
        }
    }
    unsigned int index = rand() % fsm[*state].size();
    unsigned int i = 0;
    for ( auto it = fsm[*state].begin(); it != fsm[*state].end() ; ++it, ++i ){
        if ( i == index  ){
            *state = it->nstate;
            *varMap["out"] = it->out;
            inputSequence.push_back(it->defaultPattern());
            printOutputSequence();
            cout << "Next state: " << *state << endl;
            return;
        }
    }
}

bool recursionChecker( int nstate ){
    for ( auto it = fsm[nstate].begin() ; it != fsm[nstate].end() ; ++it ){
        if ( it->nstate == *state )
            return true;
    }
    return false;
}

void terminator(Assertion& asrt){


}

void preOperationForSimulator(){
    (*state) = 0;
    const int ptnSize = size = fsm[0].front().pattern.size();
    rstRecord.push_back(inputSequence.size());
    for ( auto it = fsm[(*state)].begin(); it != fsm[(*state)].end() ; ++it ){
        if ( it->pattern == inputSequence[inputSequence.size()-1] ){
            inputSequence.push_back(Pattern<>(ptnSize));
            (*varMap["out"]) = it->out;
            (*state) = it->nstate;
            break;
        }
    }
}

std::pair<bool,unsigned int> find( unsigned short* target ){
    std::pair<bool,unsigned int> res;
    if ( target >= &(*varMap["out"])[0] ){
        res.first = true;
        res.second = (target-&(*varMap["out"])[0])*2 / sizeof(unsigned short);
    } else {
        res.first = false;
        res.second = (target-&(*varMap["in"])[0])*2 / sizeof(unsigned short);
    }
    return res;
}

void printInputSequence(){
    cout << "Result: \n";
    for ( auto it = inputSequence.begin() ; it != inputSequence.end() ; ++it )
        cout << *it << endl;
}

void printOutputSequence(){
    cout << "Current output sequence: ";
    for ( unsigned int i = 0 ; i < varMap["out"]->size() ; ++i){
        cout << (*varMap["out"])[i];
    }
    cout << endl;
}
