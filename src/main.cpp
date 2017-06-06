#include "SVParser.hpp"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <vector>

using namespace SVParser;
using std::cout;
using std::endl;

extern int yyparse(void);

extern std::map< std::string, Pattern<>* > varMap;
extern std::map< std::string, unsigned int > parameterTable;
extern std::map< int, std::list< Transition > > fsm;
extern std::list< Assertion > asrtList;

struct ActivativePoint{
    int state1, state2;
    Transition transition1, transition2;
};

struct Direction {
    int state;
    Transition* transition;
};

int ptnSize;
int* state = new int;
std::vector< Pattern<> > inputSequence;
std::vector< unsigned int > rstRecord;
std::map<int , std::list<Transition>::iterator> fsmIt;

void initializer();
void simulator();
void activator(Assertion&);
void staticFindOutputSignalActivativePoint( Assertion& );
bool traversalChecker(std::list<Direction>&, Transition* transition);
bool outputSignalActivator( unsigned int, bool, Transition&);
bool outActivate(unsigned int, unsigned int);
bool recursionChecker(int);
void terminator(Assertion&);
void reset();
void preOperationForSimulator();
std::pair< bool, unsigned int > find(unsigned short*);
void printInputSequence();
void printOutputSequence();
int main(int argc, const char* argv[])
{
    yyparse();
    ptnSize = fsm[0].front().pattern.size();
    for ( int i = 0 ; i < fsm.size() ; ++i ){
        fsmIt[i] = fsm[i].begin();
    }
    simulator();
    for (auto it = varMap.begin(); it != varMap.end(); ++it) {
        delete it->second;
    }
    delete state;
    return EXIT_SUCCESS;
}

void initializer(){
    inputSequence.clear();
    inputSequence.push_back(Pattern<>(ptnSize));
    (*state) = 0;
}

void simulator()
{
    int count = 0;
    for (auto it = asrtList.begin(); it != asrtList.end(); ++it) {
        cout << "Assertion: " << ++count << endl;
        initializer();
        staticFindOutputSignalActivativePoint(*it);
        //activator(*it);
        //reset();
    }
    printInputSequence();
}

void staticFindOutputSignalActivativePoint( Assertion& asrt ){
    std::pair<bool, unsigned int> io = find(asrt.trigger.target);
    unsigned int index = io.second;
    std::list<ActivativePoint> APList;
    cout << "Activative target: " << (( io.first ) ? "out[" : "in[" ) << index << "]"
        << " is " << (asrt.trigger.change ? "rose" : "fell") << "." << endl;
    if ( io.first ){
        cout << "output-signal-activated assertion." << endl;
        for ( int i = 0 ; i < fsm.size() ; ++i ){
            for ( auto it1 = fsm[i].begin() ; it1 != fsm[i].end(); ++it1 ){
                if ( it1->out[ptnSize-1-index] == !asrt.trigger.change ){
                    for ( auto it2 = fsm[it1->nstate].begin(); it2 != fsm[it1->nstate].end(); ++it2 ){
                        if ( it2->out[ptnSize-1-index] == asrt.trigger.change ){
                            APList.push_back(ActivativePoint({i,it1->nstate,*it1,*it2}));
                        }
                    }
                }
            }
        }
    } else {
        cout << "input-signal-activated assertion." << endl;
    }
    for ( auto it = APList.begin() ; it != APList.end(); ++it ){
        cout << "state1: " << it->state1 << ", " << "state2: " << it->state2 << ", "
             << "transition1: " << it->transition1.pattern << ", " << "transition2: " << it->transition2.pattern <<endl;
    }
    cout << endl << endl;
}

void activator(Assertion& asrt)
{
    std::pair<bool, unsigned int> io = find(asrt.trigger.target);
    cout << "Activative target: " << (( io.first ) ? "out[" : "in[" ) << io.second << "]"
        << " is " << (asrt.trigger.change ? "rose" : "fell") << "." << endl;
    std::list<std::list<Direction>> paths;
    std::list<Direction> stack;

    if ( io.first ){
        cout << "output-signal-activated assertion." << endl;
        cout << "current state: " << *state << endl;
        cout << "transition: " << fsm[*state].front().pattern
             << ", then go to state " << fsm[*state].front().nstate;
        rstRecord.push_back(inputSequence.size());
        inputSequence.push_back(fsm[*state].front().defaultPattern());
        Direction temp = Direction({*state,&(fsm[*state].front())});
        *state = fsm[*state].front().nstate;
        *varMap["out"] = fsm[*state].front().out;
        cout << ", out is " << *varMap["out"] << endl;
        stack.push_back(temp);
        while (stack.size()) {
            cout << endl;
            bool pop_back = true;
            cout << "current state: " << *state << endl;
            // if ( fsmIt[*state] == fsm[*state].end() ){
            //     fsmIt[*state] = fsm[*state].begin();
            //     stack.pop_back();
            // }
            // else {
            //     inputSequence.push_back(fsmIt[*state]->defaultPattern());
            //     stack.push_back(Direction({*state,&(*fsmIt[*state])}));
            //     if ( outputSignalActivator(io.second, asrt.trigger.change, *fsmIt[*state]) ){
            //         cout << "transition: " << fsmIt[*state]->pattern
            //         << ", then go to state " << fsmIt[*state]->nstate;
            //         cout << ", out is " << fsmIt[*state]->out << endl;
            //         cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
            //         cout << "Assertion has been activated." << endl;
            //         cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
            //         paths.push_back(stack);
            //         stack.pop_back();
            //     }
            //     unsigned int buf = *state;
            //     *state = fsmIt[buf]->nstate;
            //     *varMap["out"] = fsmIt[buf]->out;
            //     cout << "transition: " << fsmIt[buf]->pattern
            //         << ", then go to state " << *state;
            //     cout << ", out is " << *varMap["out"] << endl;
            //     fsmIt[buf]++;
            // }
            for ( auto it = fsm[*state].begin() ; it != fsm[*state].end() ; ++it ){
                if ( !traversalChecker( stack, &(*it) ) ){
                    inputSequence.push_back(it->defaultPattern());
                    stack.push_back(Direction({*state,&(*it)}));
                    if ( outputSignalActivator(io.second, asrt.trigger.change, *it) ){
                        cout << "transition: " << it->pattern
                             << ", then go to state " << it->nstate;
                        cout << ", out is " << it->out << endl;
                        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
                        cout << "Assertion has been activated." << endl;
                        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
                        paths.push_back(stack);
                        break;
                    }
                    *state = it->nstate;
                    *varMap["out"] = it->out;
                    cout << "transition: " << it->pattern
                         << ", then go to state " << *state;
                    cout << ", out is " << *varMap["out"] << endl;
                    pop_back = false;
                    break;
                }
            }
            if ( pop_back )
                stack.pop_back();
        }
    } else {
        cout << "input-signal-activated assertion." << endl;
    }
}

bool traversalChecker( std::list<Direction>& stack , Transition* transition  ){
    for ( auto it = stack.begin() ; it != stack.end() ; ++it ){
        if ( it->transition == transition )
            return true;
    }
    return false;
}

bool outputSignalActivator( unsigned int index, bool triggerFlag, Transition& transition ){
    cout << "outputSignalActivator para: " << index << " " << triggerFlag << endl;
    cout << "current out[" << index << "] is " << (*varMap["out"])[ptnSize-1-index] << endl;
    cout << "next out[" << index << "] is " << transition.out[ptnSize-1-index] << endl;
    if ( (*varMap["out"])[ptnSize-1-index] == !triggerFlag && transition.out[ptnSize-1-index] == triggerFlag )
        return true;
    return false;
}

bool outActivate(unsigned int index, unsigned int triggerFlag)
{

    cout << "Out Activate start.\n";
    cout << "Current state: " << *state << endl;
    for (auto it = fsm[*state].begin(); it != fsm[*state].end(); ++it) {
        //first meet
        if (it->out[index] == triggerFlag && !it->traversed) {
            cout << "Assertion has been activated!\n";
            it->traversed = true;
            *state = it->nstate;
            *varMap["out"] = it->out;
            inputSequence.push_back(it->defaultPattern());
            printOutputSequence();
            cout << "Next state: " << *state << endl;
            return true;
        } else if (it->out[index] == triggerFlag) {
            cout << "Assertion has been activated!\n";
            unsigned int idx = rand() % fsm[*state].size();
            unsigned int i = 0;
            for (auto rand_it = fsm[*state].begin(); rand_it != fsm[*state].end(); ++rand_it, ++i) {
                if (i == idx) {
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
    for (auto it = fsm[*state].begin(); it != fsm[*state].end(); ++it) {
        if (*state != it->nstate && !it->traversed) {
            it->traversed = true;
            *state = it->nstate;
            *varMap["out"] = it->out;
            inputSequence.push_back(it->defaultPattern());
            printOutputSequence();
            cout << "Next state: " << *state << endl;
            break;
        } else if (*state != it->nstate) {
            unsigned int idx = rand() % fsm[*state].size();
            unsigned int i = 0;
            for (auto rand_it = fsm[*state].begin(); rand_it != fsm[*state].end(); ++rand_it, ++i) {
                if (i == idx) {
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

void reset()
{
    cout << "Reset." << endl;
    rstRecord.push_back(inputSequence.size());
    *state = 0;
    for (auto it = fsm[*state].begin(); it != fsm[*state].end(); ++it) {
        if (!it->traversed) {
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
    for (auto it = fsm[*state].begin(); it != fsm[*state].end(); ++it, ++i) {
        if (i == index) {
            *state = it->nstate;
            *varMap["out"] = it->out;
            inputSequence.push_back(it->defaultPattern());
            printOutputSequence();
            cout << "Next state: " << *state << endl;
            return;
        }
    }
}

bool recursionChecker(int nstate)
{
    for (auto it = fsm[nstate].begin(); it != fsm[nstate].end(); ++it) {
        if (it->nstate == *state)
            return true;
    }
    return false;
}

void terminator(Assertion& asrt)
{
}

void preOperationForSimulator()
{
    (*state) = 0;
    rstRecord.push_back(inputSequence.size());
    for (auto it = fsm[(*state)].begin(); it != fsm[(*state)].end(); ++it) {
        if (it->pattern == inputSequence[inputSequence.size() - 1]) {
            inputSequence.push_back(Pattern<>(ptnSize));
            (*varMap["out"]) = it->out;
            (*state) = it->nstate;
            break;
        }
    }
}

std::pair< bool, unsigned int > find(unsigned short* target)
{
    std::pair< bool, unsigned int > res;
    if (target >= &(*varMap["out"])[0]) {
        res.first = true;
        res.second = (target - &(*varMap["out"])[0]) * 2 / sizeof(unsigned short);
    } else {
        res.first = false;
        res.second = (target - &(*varMap["in"])[0]) * 2 / sizeof(unsigned short);
    }
    return res;
}

void printInputSequence()
{
    cout << "Result: \n";
    for (auto it = inputSequence.begin(); it != inputSequence.end(); ++it)
        cout << *it << endl;
}

void printOutputSequence()
{
    cout << "Current output sequence: ";
    for (unsigned int i = 0; i < varMap["out"]->size(); ++i) {
        cout << (*varMap["out"])[i];
    }
    cout << endl;
}
