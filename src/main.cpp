#include "SVParser.hpp"
#include <climits>
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

extern std::map< std::string, Pattern* > varMap;
extern std::list< Assertion > asrtList;
extern FiniteStateMachine FSM;

int ptnSize;
int* state = new int;
std::vector< Pattern > inputSequence;
std::vector< unsigned int > rstRecord;
std::vector< int > layerTable;
std::map< int, std::list< int > > rlayerTable;

void preProcessor();
void initializer();
void simulator();
void staticFindActivatedPoint(Assertion&);
void staticFindOutputSignalActivatedPoint(bool, unsigned int, std::list< ActivatedPoint >&);
void staticFindInputSignalActivatedPoint(bool, unsigned int, std::list< ActivatedPoint >&);
void iterativelyEvalStateLayer();
void fromActivatedPoint2AssertionFailed();
std::pair< bool, unsigned int > find(unsigned short*);
void printInputSequence();
void printOutputSequence();
void printStateLayer(int);
int main(int argc, const char* argv[])
{
    yyparse();
    preProcessor();
    //printStateLayer();
    simulator();
    int count = 0;
    for (auto it = asrtList.begin(); it != asrtList.end(); ++it) {
        cout << "Assertion: " << ++count << endl;
        for (auto itt = it->APList.begin(); itt != it->APList.end(); ++itt) {
            cout << itt->state->layer << endl;
        }
    }
    for (auto it = varMap.begin(); it != varMap.end(); ++it) {
        delete it->second;
    }
    delete state;
    return EXIT_SUCCESS;
}

void preProcessor()
{
    // record pattern size
    ptnSize = FSM.inputSize();
    // initialize each state's layer = 2147483647
    layerTable.assign(FSM.size(), INT_MAX);
    layerTable[0] = 0;
    // initialize initial layer S0
    rlayerTable[0] = std::move(std::list< int >(1, 0));

    iterativelyEvalStateLayer();
    printStateLayer(true);
    // FSM.printStateLayer();
}

void initializer()
{
    inputSequence.clear();
    inputSequence.push_back(Pattern(ptnSize));
    (*state) = 0;
}

void simulator()
{
    int count = 0;
    for (auto it = asrtList.begin(); it != asrtList.end(); ++it) {
        cout << "Assertion: " << ++count << endl;
        initializer();
        staticFindActivatedPoint(*it);
        it->printActivatedPoint();
    }
}

void staticFindActivatedPoint(Assertion& asrt)
{
    std::pair< bool, unsigned int > io = find(asrt.trigger.target);
    bool triggerFlag = io.first;
    unsigned int index = io.second;
    cout << "Activated target: " << ((triggerFlag) ? "out[" : "in[") << index << "]"
         << " is " << (triggerFlag ? "rose" : "fell") << "." << endl;
    if (triggerFlag)
        staticFindOutputSignalActivatedPoint(triggerFlag, index, asrt.APList);
    else
        staticFindInputSignalActivatedPoint(triggerFlag, index, asrt.APList);
}

void staticFindOutputSignalActivatedPoint(bool triggerFlag, unsigned int index, std::list< ActivatedPoint >& APList)
{
    cout << "output-signal-activated assertion." << endl;
    for (int i = 0; i < FSM.size(); ++i) {
        for (auto it1 = FSM[i]->transitions.begin(); it1 != FSM[i]->transitions.end(); ++it1) {
            if ((*it1)->out[index] == !triggerFlag) {
                for (auto it2 = FSM[(*it1)->nState->label]->transitions.begin(); it2 != FSM[(*it1)->nState->label]->transitions.end(); ++it2) {
                    if ((*it2)->out[index] == triggerFlag) {
                        APList.push_back(ActivatedPoint({ FSM[i], (*it1)->defaultPattern(), (*it2)->defaultPattern(), *it1, *it2 }));
                    }
                }
            }
        }
    }
}

void staticFindInputSignalActivatedPoint(bool triggerFlag, unsigned int index, std::list< ActivatedPoint >& APList)
{
    cout << "input-signal-activated assertion." << endl;
    for (int i = 0; i < FSM.size(); ++i) {
        for (auto it1 = FSM[i]->transitions.begin(); it1 != FSM[i]->transitions.end(); ++it1) {
            Pattern expectedPattern1 = (*it1)->pattern;
            if (expectedPattern1[index] == !triggerFlag || expectedPattern1[index] == 2)
                expectedPattern1[index] = !triggerFlag;
            else
                continue;
            if ((*it1)->pattern == expectedPattern1) {
                for (auto it2 = FSM[(*it1)->nState->label]->transitions.begin(); it2 != FSM[(*it1)->nState->label]->transitions.end(); ++it2) {
                    Pattern expectedPattern2 = (*it2)->pattern;
                    if (expectedPattern2[index] == triggerFlag || expectedPattern2[index] == 2)
                        expectedPattern2[index] = triggerFlag;
                    else
                        continue;
                    if ((*it2)->pattern == expectedPattern2) {
                        APList.push_back(ActivatedPoint({ FSM[i], expectedPattern1, expectedPattern2, *it1, *it2 }));
                    }
                }
            }
        }
    }
}

void iterativelyEvalStateLayer()
{
    std::list< int > queue;
    queue.push_back(0);
    while (queue.size()) {
        FSM[queue.front()]->traversed = true;
        for (auto it = FSM[queue.front()]->transitions.begin(); it != FSM[queue.front()]->transitions.end(); ++it) {
            if (layerTable[queue.front()] + 1 < layerTable[(*it)->nState->label] && (*it)->nState->label != queue.back()) {
                layerTable[(*it)->nState->label] = layerTable[queue.front()] + 1;
                if (rlayerTable.find(layerTable[queue.front()] + 1) != rlayerTable.end()) {
                    rlayerTable[layerTable[queue.front()] + 1].push_back((*it)->nState->label);
                    (*it)->nState->layer = layerTable[queue.front()] + 1;
                } else {
                    std::list< int > stateList;
                    stateList.push_back((*it)->nState->label);
                    rlayerTable[layerTable[queue.front()] + 1] = stateList;
                }
                queue.push_back((*it)->nState->label);
            }
        }
        queue.pop_front();
    }
}

void fromActivatedPoint2AssertionFailed(Assertion& asrt)
{

    std::pair< bool, unsigned int > io = find(asrt.trigger.target);
    bool triggerFlag = io.first;
    unsigned int index = io.second;
    for (auto APit = asrt.APList.begin(); APit != asrt.APList.end(); ++APit) {
        std::list< int > queue;
        queue.push_back(APit->state->label);
        while (queue.size()) {
            for (auto it = FSM[queue.front()]->transitions.begin(); it != FSM[queue.front()]->transitions.end(); ++it) {
            }
            queue.pop_front();
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

void printStateLayer(int mode)
{
    if (mode) {
        int count = 0;
        for (auto it = layerTable.begin(); it != layerTable.end(); ++it) {
            cout << "S" << count << ": " << *it << (!FSM[count]->traversed ? "X" : "") << endl;
            count++;
        }
    } else {
        for (auto it = rlayerTable.begin(); it != rlayerTable.end(); ++it) {
            cout << "Layer" << it->first << ": ";
            for (auto stateIt = it->second.begin(); stateIt != it->second.end(); ++stateIt)
                cout << *stateIt << ", ";
            cout << endl;
        }
    }
}
