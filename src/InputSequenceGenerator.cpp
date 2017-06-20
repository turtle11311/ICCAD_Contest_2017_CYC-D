#include "InputSequenceGenerator.hpp"
#include <map>
#include <queue>

namespace SVParser {

InputSequenceGenerator::InputSequenceGenerator()
    : _Base()
{
    ::yyparse(*this);
}

void InputSequenceGenerator::preprocess()
{
    evalInitial2State();
    // purge unreach state
    for (auto it = this->begin(); it != this->end(); ++it)
        if (!it->second->traversed)
            purgeState(it->first);

    for (auto& asrt : asrtList) {
        staticFindActivatedPoint(asrt);
        asrt.sortActivatedPointByLayer();
    }
}

void InputSequenceGenerator::simulator()
{
    cout << "Simulator!\n";
    int count = 0;
    for (Assertion& asrt : asrtList) {
        cout << "Assertion: " << ++count << endl;
        path.clear();
        asrtFailedFlag = false;
        fromActivatedPoint2AssertionFailed(asrt);
        cout << endl
             << endl;
    }
}

void InputSequenceGenerator::fromActivatedPoint2AssertionFailed(Assertion& asrt)
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

void InputSequenceGenerator::findOutputSignalTermiateStartPoint(bool triggerFlag, unsigned int index, ActivatedPoint& ap, Range& range)
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

void InputSequenceGenerator::recursiveTraverseOS(std::list< ActivatedPoint > stack, bool triggerFlag, unsigned int index, unsigned int cycle)
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

void InputSequenceGenerator::findInputSignalTermiateStartPoint(bool triggerFlag, unsigned int index, ActivatedPoint& ap, Range& range)
{
}

void InputSequenceGenerator::usingDijkstraForNonWeightedGraph(ActivatedPoint& ap)
{
    resetTraversed();
    State* goal = ap.state;
    std::list< std::list< int > > queue;
    queue.push_back(std::list< int >(1, 0));
    while (queue.size()) {
        State* cur = getState(queue.back().front());
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

std::list< ActivatedPoint > InputSequenceGenerator::integerPath2APPathConverter(std::list< int >& shortestPath, ActivatedPoint& ap)
{
    cout << shortestPath.size() << endl;
    std::list< ActivatedPoint > temp;
    auto it = shortestPath.begin();
    ++it;
    auto _it = shortestPath.begin();
    for (State::From& from : getState(*it)->fromList) {
        if (from.state == getState(*_it)) {
            temp.push_back(ActivatedPoint({ getState(*_it), from.transition->pattern, from.transition->pattern, from.transition, from.transition }));
            break;
        }
    }
    ++_it, ++it;
    for (; it != shortestPath.end(); ++it) {
        for (State::From& from : getState(*it)->fromList) {
            if (from.state == getState(*_it)) {
                temp.back().pattern2 = from.transition->pattern;
                temp.back().transition2 = from.transition;
                temp.push_back(ActivatedPoint({ getState(*_it), from.transition->pattern, from.transition->pattern, from.transition, from.transition }));
            }
        }
    }
    temp.back().pattern2 = ap.pattern1;
    temp.back().transition2 = ap.transition1;
    cout << temp.size() << endl;
    return temp;
}

void InputSequenceGenerator::printPath()
{
    cout << "Path: " << endl;
    for (auto it = path.begin(); it != path.end(); ++it) {
        cout << it->pattern1 << " -> S" << it->state->label << " -> " << it->transition1->out;
        cout << " => ";
        cout << it->pattern2 << " -> S" << it->transition1->nState->label << " -> " << it->transition2->out << endl;
        cout << "|" << endl;
    }
}

void InputSequenceGenerator::evalInitial2State()
{
    std::queue< State* > bfsQueue;
    State* S0 = this->getState(0);
    S0->layer = 0;
    bfsQueue.push(S0);
    initial2ActivetedList[0] = std::move(InputSequence(1, Pattern(inputSize())));
    bfsQueue.front()->traversed = true;
    while (bfsQueue.size()) {
        for (Transition* transition : bfsQueue.front()->transitions) {
            State* next = transition->nState;
            if (!next->traversed) {
                next->layer = bfsQueue.front()->layer + 1;
                initial2ActivetedList[next->label] = initial2ActivetedList[bfsQueue.front()->label];
                initial2ActivetedList[next->label].push_back(transition->defaultPattern());
                bfsQueue.push(next);
                next->traversed = true;
            }
        }
        bfsQueue.pop();
    }
}

void InputSequenceGenerator::staticFindActivatedPoint(Assertion& asrt)
{
    bool signalFlag = asrt.trigger.target == TargetType::OUT ? true : false;
    bool trigger = asrt.trigger.change == SignalEdge::ROSE ? true : false;
    unsigned int index = asrt.trigger.index;
    // cout << "Activated target: " << ((signalFlag) ? "out[" : "in[") << index << "]"
    //      << " is " << (triggerFlag ? "rose" : "fell") << "." << endl;
    if (signalFlag)
        staticFindOutputSignalActivatedPoint(trigger, index, asrt.APList);
    else
        staticFindInputSignalActivatedPoint(trigger, index, asrt.APList);
}

void InputSequenceGenerator::staticFindOutputSignalActivatedPoint(bool trigger, unsigned int index, std::list< ActivatedPoint >& APList)
{
    for (auto& state : (*this)) {
        for (Transition* trans1 : state.second->transitions) {
            if (trans1->out[index] == !trigger) {
                for (Transition* trans2 : getState(trans1->nState->label)->transitions) {
                    if (trans2->out[index] == trigger) {
                        APList.push_back(ActivatedPoint({ getState(state.second->label), trans1->pattern, trans2->pattern, trans1, trans2 }));
                    }
                }
            }
        }
    }
}

void InputSequenceGenerator::staticFindInputSignalActivatedPoint(bool trigger, unsigned int index, std::list< ActivatedPoint >& APList)
{
    for (auto& state : (*this)) {
        for (Transition* trans1 : state.second->transitions) {
            Pattern expectedPattern1 = trans1->pattern;
            if (expectedPattern1[index] == !trigger || expectedPattern1[index] == 2)
                expectedPattern1[index] = !trigger;
            else
                continue;

            if (trans1->pattern == expectedPattern1) {
                for (Transition* trans2 : getState(trans1->nState->label)->transitions) {
                    Pattern expectedPattern2 = trans2->pattern;
                    if (expectedPattern2[index] == trigger || expectedPattern2[index] == 2)
                        expectedPattern2[index] = trigger;
                    else
                        continue;
                    if (trans2->pattern == expectedPattern2) {
                        APList.push_back(ActivatedPoint({ getState(state.second->label), expectedPattern1, expectedPattern2, trans1, trans2 }));
                    }
                }
            }
        }
    }
}

void InputSequenceGenerator::purgeState(int state)
{
    auto it = this->find(state);
    if (it != this->end()) {
        delete it->second;
        this->erase(it);
    }
}
}
