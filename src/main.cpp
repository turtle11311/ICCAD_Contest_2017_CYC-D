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
void printInputSequence();
void printActivatedPoint(int);
void printStateLayer();
int main(int argc, const char* argv[])
{
    yyparse();
    preProcessor();
    printStateLayer();
    simulator();
    // printActivatedPoint(1);

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
    FSM[0]->layer = 0;
    // initialize initial layer S0
    rlayerTable[0] = std::move(std::list< int >(1, 0));

    iterativelyEvalStateLayer();
    // printStateLayer(true);
    FSM.printStateLayer();
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
        it->sortActivatedPointByLayer();
        it->printActivatedPoint();
    }
}

void staticFindActivatedPoint(Assertion& asrt)
{
    bool signalFlag = asrt.trigger.target == TargetType::OUT ? true : false;
    bool triggerFlag = asrt.trigger.change == SignalEdge::ROSE ? true : false;
    unsigned int index = asrt.trigger.index;
    cout << "Activated target: " << ((signalFlag) ? "out[" : "in[") << index << "]"
         << " is " << (triggerFlag ? "rose" : "fell") << "." << endl;
    if (signalFlag)
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
                        APList.push_back(ActivatedPoint({ FSM[i], (*it1)->pattern, (*it2)->pattern, *it1, *it2 }));
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
    int count = 0;
    while (queue.size()) {
        FSM[queue.front()]->traversed = true;
        for (auto it = FSM[queue.front()]->transitions.begin(); it != FSM[queue.front()]->transitions.end(); ++it) {
            if ((*it)->nState->label == 5)
                count++;
            if (layerTable[queue.front()] + 1 < layerTable[(*it)->nState->label] && (*it)->nState->label != queue.back()) {
                layerTable[(*it)->nState->label] = layerTable[queue.front()] + 1;
                (*it)->nState->layer = layerTable[queue.front()] + 1;
                if (rlayerTable.find(layerTable[queue.front()] + 1) != rlayerTable.end()) {
                    rlayerTable[layerTable[queue.front()] + 1].push_back((*it)->nState->label);
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
    cout << count << "!!!!!!!!!!!!!!!!!!\n";
}

void fromActivatedPoint2AssertionFailed(Assertion& asrt)
{

    bool triggerFlag = asrt.trigger.target == TargetType::OUT ? true : false;
    unsigned int index = asrt.trigger.index;
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

void printInputSequence()
{
    cout << "Result: \n";
    for (auto it = inputSequence.begin(); it != inputSequence.end(); ++it)
        cout << *it << endl;
}

void printActivatedPoint(int mode)
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
    cout << endl
         << endl;
}

void printStateLayer()
{
    for (auto it = rlayerTable.begin(); it != rlayerTable.end(); ++it) {
        cout << "Layer" << it->first << ": ";
        for (auto stateIt = it->second.begin(); stateIt != it->second.end(); ++stateIt)
            cout << *stateIt << ", ";
        cout << endl;
    }
}
