#include "SVParser.hpp"
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <vector>

using namespace SVParser;
using std::cout;
using std::endl;

extern int yyparse(void);
extern FILE* yyin;
extern std::list< Assertion > asrtList;
extern FiniteStateMachine FSM;

std::ofstream output;

int ptnSize;
int* state = new int;
std::vector< Pattern > inputSequence;
std::vector< unsigned int > rstRecord;
std::vector< int > layerTable;
std::map< int, std::list< int > > rlayerTable;
std::list< ActivatedPoint > path;
bool asrtFailedFlag = false;

void parseArgAndInitial(int argc, char* argv[]);

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
void usingDijkstraForNonWeightedGraph(ActivatedPoint& ap);
std::list< ActivatedPoint > integerPath2APPathConverter(std::list< int >&, ActivatedPoint&);
void printInputSequence();
void printActivatedPoint(int);
void printStateLayer();
void printPath();

int main(int argc, char* argv[])
{
    parseArgAndInitial(argc, argv);
    yyparse();

    // speed up c++ STL I/O
    std::ios_base::sync_with_stdio(false);

    preProcessor();
    simulator();
    delete state;
    return EXIT_SUCCESS;
}

void parseArgAndInitial(int argc, char* argv[])
{
    if (argc < 4) {
        fprintf(stderr, "Usage: %s -i input_file -o output_file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char opt;
    while ((opt = getopt(argc, argv, "i:o:")) != EOF) {
        switch (opt) {
        case 'i':
            yyin = fopen(optarg, "r");
            break;
        case 'o':
            output.open(optarg, std::ios::out);
            break;
        default:
            break;
        }
    }
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
    for (auto it = FSM.begin(); it != FSM.end(); ++it) {
        if (!it->second->traversed)
            FSM.setIsolatedState(it->second->label);
    }

    // FSM.printStateLayer();
    for (auto it = asrtList.begin(); it != asrtList.end(); ++it) {
        staticFindActivatedPoint(*it);
        it->sortActivatedPointByLayer();
        //it->printActivatedPoint();
    }

    output.close();
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
        if (FSM.isIsolated(i))
            continue;
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
    while (queue.size()) {
        FSM[queue.front()]->traversed = true;
        for (auto it = FSM[queue.front()]->transitions.begin(); it != FSM[queue.front()]->transitions.end(); ++it) {
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
            if (asrtFailedFlag) {
                cout << "Activated point: ";
                path.front().printAP();
                cout << endl
                     << endl;

                usingDijkstraForNonWeightedGraph(path.front());
                printPath();
                break;
            }
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

void usingDijkstraForNonWeightedGraph(ActivatedPoint& ap)
{
    FSM.resetTraversed();
    State* goal = ap.state;
    std::list< std::list< int > > queue;
    queue.push_back(std::list< int >(1, 0));
    while (queue.size()) {
        State* cur = FSM[queue.back().front()];
        for (auto it = cur->transitions.begin(); it != cur->transitions.end(); ++it) {
            std::list< int > shortestPath = queue.front();
            shortestPath.push_back((*it)->nState->label);
            if (cur->traversed)
                continue;
            if ((*it)->nState == goal) {
                std::list< ActivatedPoint > temp = integerPath2APPathConverter(shortestPath, ap);
                for (auto tempIt = temp.rbegin(); tempIt != temp.rend(); ++tempIt) {
                    path.push_front(*tempIt);
                }
                queue.clear();
                return;
            } else {
                if ((*it)->nState == cur)
                    cur->traversed = true;
                queue.push_back(shortestPath);
            }
        }
        queue.pop_front();
    }
}

std::list< ActivatedPoint > integerPath2APPathConverter(std::list< int >& shortestPath, ActivatedPoint& ap)
{
    cout << shortestPath.size() << endl;
    std::list< ActivatedPoint > temp;
    auto it = shortestPath.begin();
    ++it;
    auto _it = shortestPath.begin();
    for (auto from = FSM[*it]->fromList.begin(); from != FSM[*it]->fromList.end(); ++from) {
        if (from->state == FSM[*_it]) {
            temp.push_back(ActivatedPoint({ FSM[*_it], from->transition->pattern, from->transition->pattern, from->transition, from->transition }));
            break;
        }
    }
    ++_it, ++it;
    for (; it != shortestPath.end(); ++it) {
        for (auto from = FSM[*it]->fromList.begin(); from != FSM[*it]->fromList.end(); ++from) {
            if (from->state == FSM[*_it]) {
                temp.back().pattern2 = from->transition->pattern;
                temp.back().transition2 = from->transition;
                temp.push_back(ActivatedPoint({ FSM[*_it], from->transition->pattern, from->transition->pattern, from->transition, from->transition }));
            }
        }
    }
    temp.back().pattern2 = ap.pattern1;
    temp.back().transition2 = ap.transition1;
    cout << temp.size() << endl;
    return temp;
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
