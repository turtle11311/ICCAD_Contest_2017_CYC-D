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
std::list< ActivatedPoint > path;
bool asrtFailedFlag = false;

void preProcessor();
void initializer();
void simulator();
void staticFindActivatedPoint(Assertion&);
void staticFindOutputSignalActivatedPoint(bool, unsigned int, std::list< ActivatedPoint >&);
void staticFindInputSignalActivatedPoint(bool, unsigned int, std::list< ActivatedPoint >&);
void iterativelyEvalStateLayer();
void fromActivatedPoint2AssertionFailed(Assertion&);
void findOutputSignalTermiateStartPoint(bool, unsigned int, ActivatedPoint&, Range&);
void findInputSignalTermiateStartPoint(bool, unsigned int, ActivatedPoint&, Range&);
void recursiveTraverseOS(std::list< ActivatedPoint >, bool, unsigned int, unsigned int);
void printInputSequence();
void printActivatedPoint(int);
void printStateLayer();
void printPath();
int main(int argc, const char* argv[])
{
    yyparse();
    preProcessor();
    simulator();
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
    // FSM.printStateLayer();

    for (auto it = asrtList.begin(); it != asrtList.end(); ++it) {
        staticFindActivatedPoint(*it);
        it->sortActivatedPointByLayer();
        //it->printActivatedPoint();
    }
}

void initializer()
{
    inputSequence.clear();
    inputSequence.push_back(Pattern(ptnSize));
    (*state) = 0;
}

void simulator()
{
    cout << "Simulator!\n";
    int count = 0;
    for (auto it = asrtList.begin(); it != asrtList.end(); ++it) {
        cout << "Assertion: " << ++count << endl;
        path.clear();
        asrtFailedFlag = false;
        fromActivatedPoint2AssertionFailed(*it);
        cout << endl
             << endl;
    }
}

void staticFindActivatedPoint(Assertion& asrt)
{
    bool signalFlag = asrt.trigger.target == TargetType::OUT ? true : false;
    bool triggerFlag = asrt.trigger.change == SignalEdge::ROSE ? true : false;
    unsigned int index = asrt.trigger.index;
    // cout << "Activated target: " << ((signalFlag) ? "out[" : "in[") << index << "]"
    //      << " is " << (triggerFlag ? "rose" : "fell") << "." << endl;
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
}

void fromActivatedPoint2AssertionFailed(Assertion& asrt)
{
    bool signalFlag = asrt.event.target == TargetType::OUT ? true : false;
    bool triggerFlag = asrt.event.change == SignalEdge::ROSE ? true : false;
    unsigned int index = asrt.event.index;
    cout << "Terminate target: " << ((signalFlag) ? "out[" : "in[") << index << "]"
         << " is " << (triggerFlag ? "rose" : "fell")
         << " in range[" << asrt.time.first << ":" << asrt.time.second << "]"
         << "." << endl;
    if (signalFlag) {
        for (auto it = asrt.APList.begin(); it != asrt.APList.end(); ++it) {
            findOutputSignalTermiateStartPoint(triggerFlag, index, *it, asrt.time);
            if (path.size() > 1)
                recursiveTraverseOS(std::list< ActivatedPoint >(1, path.back()), triggerFlag, index, asrt.time.length());
            if (asrtFailedFlag)
                break;
        }
    } else {
        for (auto it = asrt.APList.begin(); it != asrt.APList.end(); ++it)
            findInputSignalTermiateStartPoint(triggerFlag, index, *it, asrt.time);
    }
}

void findOutputSignalTermiateStartPoint(bool triggerFlag, unsigned int index, ActivatedPoint& ap, Range& range)
{
    path = std::list< ActivatedPoint >(1, ap);
    unsigned int start = range.first - 1;
    bool selfCycle = false;
    Transition* SCT; // self cycle Transition
    State* nState = ap.transition2->nState;
    for (auto it = nState->transitions.begin(); it != nState->transitions.end(); ++it) {
        if ((*it)->nState == nState) {
            selfCycle = true;
            SCT = *it;
            break;
        }
    }
    if (selfCycle) {
        for (int i = 0; i < start; ++i)
            path.push_back(ActivatedPoint({ nState, SCT->pattern, SCT->pattern, SCT, SCT }));
    } else {
        for (int i = 0; i < start; ++i) {
            path.push_back(
                ActivatedPoint({ nState,
                    ap.pattern2,
                    nState->transitions.front()->pattern,
                    ap.transition2,
                    nState->transitions.front() }));
            nState = nState->transitions.front()->nState;
        }
    }
}

void recursiveTraverseOS(std::list< ActivatedPoint > stack, bool triggerFlag, unsigned int index, unsigned int cycle)
{
    if (asrtFailedFlag)
        return;
    if (!cycle) {
        asrtFailedFlag = true;
        stack.pop_front();
        for (auto it = stack.begin(); it != stack.end(); ++it) {
            path.push_back(*it);
        }
        cout << "find!\n";
        printPath();
        return;
    }
    Pattern out = stack.back().transition2->out;
    State* nState = stack.back().transition2->nState;
    for (auto it = nState->transitions.begin(); it != nState->transitions.end(); ++it) {
        if (out[index] == !triggerFlag && (*it)->out[index] == !triggerFlag) {
            stack.push_back(
                ActivatedPoint({ nState,
                    stack.back().pattern2,
                    (*it)->pattern,
                    stack.back().transition2,
                    *it }));
            recursiveTraverseOS(stack, triggerFlag, index, cycle - 1);
        } else if (out[index] == triggerFlag) {
            stack.push_back(
                ActivatedPoint({ nState,
                    stack.back().pattern2,
                    (*it)->pattern,
                    stack.back().transition2,
                    *it }));
            recursiveTraverseOS(stack, triggerFlag, index, cycle - 1);
        }
    }
}

void findInputSignalTermiateStartPoint(bool triggerFlag, unsigned int index, ActivatedPoint& ap, Range& range)
{
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

void printPath()
{
    cout << "Path: " << endl;
    for (auto it = path.begin(); it != path.end(); ++it) {
        cout << it->pattern1 << " -> S" << it->state->label << " -> " << it->transition1->out;
        cout << " => ";
        cout << it->pattern2 << " -> S" << it->transition1->nState->label << " -> " << it->transition2->out << endl;
        cout << "|" << endl;
    }
}
