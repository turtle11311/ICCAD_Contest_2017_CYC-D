#include "InputSequenceGenerator.hpp"
#include <algorithm>
#include <fstream>
#include <map>
#include <queue>
extern std::ofstream output;

namespace SVParser {

InputSequenceGenerator::InputSequenceGenerator()
    : _Base()
    , found(false)
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
    cout << "Activated target: " << ((asrt.trigger.target == TargetType::OUT) ? "out[" : "in[")
         << asrt.trigger.index << "]"
         << " is " << (asrt.trigger.change == SignalEdge::ROSE ? "rose" : "fell")
         << endl;
    cout << "Terminate target: " << ((signalFlag) ? "out[" : "in[") << index << "]"
         << " is " << (triggerFlag ? "rose" : "fell")
         << " in range[" << asrt.time.first << ":" << asrt.time.second << "]"
         << "." << endl;

    bool res = false;
    if (signalFlag) {
        for (ActivatedPoint& ap : asrt.APList) {
            res = fromActivatedPoint2AssertionOutputSignalFailed(asrt, answerDict[&asrt], ap.transition2->nState, ap.transition2, 0);
            if (res) {
                targetAP = ap;
                break;
            }
        }
    }
    cout << "Assertion " << (res ? "Fail" : "Success") << endl;
    if (res) {
        recPath.push_back(getState(0));
        recursiveDFS();
        recPath.pop_front();
        for (Transition* trans : (*this)[0]->transitions) {
            if (trans->nState == recPath.front())
                firstHalfAnswer.push_front(trans->defaultPattern());
        }
        for (auto pit = firstHalfAnswer.rbegin(); pit != firstHalfAnswer.rend(); ++pit) {
            answerDict[&asrt].push_front(*pit);
        }
    }
    recPath.clear();
    firstHalfAnswer.clear();
}

bool InputSequenceGenerator::fromActivatedPoint2AssertionOutputSignalFailed(Assertion& asrt, InputSequence& sequence, State* current, Transition* t2, size_t step)
{
    sequence.push_back(t2->defaultPattern());
    if (step == asrt.time.second) {
        sequence.pop_back();
        return false;
    }
    if (step >= asrt.time.first) {
        bool income = false, outcome = false;
        size_t index = asrt.event.index;
        for (State::From& from : current->fromList) {
            if (from.transition->out[index] == (asrt.event.change == SignalEdge::FELL ? 0 : 1)) {
                income = true;
                break;
            }
        }
        for (Transition* transition : current->transitions) {
            if (transition->out[index] == (asrt.event.change == SignalEdge::FELL ? 1 : 0)) {
                outcome = true;
                break;
            }
        }
        if (income && outcome) {
            cout << step << endl;
            return true;
        }
    }
    for (auto trans = current->transitions.begin(); trans != current->transitions.end(); ++trans) {
        bool res = fromActivatedPoint2AssertionOutputSignalFailed(asrt, sequence, (*trans)->nState, *trans, step + 1);
        if (res == true)
            return true;
    }
    sequence.pop_back();
    return false;
}

void InputSequenceGenerator::findOutputSignalTermiateStartPoint(bool triggerFlag, unsigned int index, ActivatedPoint& ap, Range& range)
{
    path = std::list< ActivatedPoint >(1, ap);
    unsigned int start = range.first - 1;
    bool selfCycle = false;
    Transition* SCT; // self cycle Transition
    State* nState = ap.state;
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
    std::list< State* > stack;
    stack.push_back(getState(0));
    while (!stack.empty()) {
        State* cur = stack.back();
        if (cur->tit == cur->transitions.end() || stack.size() - 1 > ap.state->layer) {
            cur->tit = cur->transitions.begin();
            stack.pop_back();
        } else {
            if (*cur->tit == ap.transition1)
                break;
            stack.push_back((*cur->tit)->nState);
            cur->tit++;
        }
    }
    if (!stack.empty()) {
        output << "!!!!!!!!!!!!!!!!!!\n";
    }
}

void InputSequenceGenerator::recursiveDFS()
{
    State* cur = recPath.back();
    for (auto it = cur->transitions.begin(); it != cur->transitions.end(); ++it) {
        if (found)
            return;
        if (*it == targetAP.transition1) {
            firstHalfAnswer.push_back(targetAP.transition1->defaultPattern());
            found = true;
            return;
        }
        if (recPath.size() - 1 < targetAP.state->layer) {
            recPath.push_back((*it)->nState);
            firstHalfAnswer.push_back((*it)->defaultPattern());
            recursiveDFS();
        }
    }
    if (found)
        return;
    firstHalfAnswer.pop_back();
    recPath.pop_back();
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
    bfsQueue.front()->traversed = true;
    while (bfsQueue.size()) {
        for (Transition* transition : bfsQueue.front()->transitions) {
            State* next = transition->nState;
            if (!next->traversed) {
                next->layer = bfsQueue.front()->layer + 1;
                if (this->rlayerTable.find(next->layer) != this->rlayerTable.end())
                    this->rlayerTable[next->layer].push_back(next);
                else
                    this->rlayerTable[next->layer] = (std::vector< State* >(1, next));
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

void InputSequenceGenerator::outputNthAssertion(int n)
{
    auto ait = std::next(asrtList.begin(), n);
    InputSequence& answer = answerDict[&(*ait)];
    output << 0 << Pattern(PATTERNSIZE) << endl;
    output << 1 << answer.front() << endl;
    for (auto iit = std::next(answer.begin()); iit != answer.end(); ++iit) {
        output << 0 << *iit << endl;
    }
}

void InputSequenceGenerator::convertPath2InputSequence()
{
    auto it = path.begin();
    answer.push_back(it->transition1->defaultPattern());
    for (; it != path.end(); ++it)
        answer.push_back(it->transition2->defaultPattern());
}

void InputSequenceGenerator::printInputSequence()
{
    for (auto it = answer.begin(); it != answer.end(); ++it)
        output << *it << endl;
    output << endl;
}

void InputSequenceGenerator::purgeState(int state)
{
    auto it = this->find(state);
    for (Transition* trans : it->second->transitions) {
        for (State::From& from : trans->nState->fromList) {
            trans->nState->fromList.erase(std::remove_if(trans->nState->fromList.begin(),
                                              trans->nState->fromList.end(),
                                              [=](State::From& from) {
                                                  return from.state == it->second;
                                              }),
                trans->nState->fromList.end());
        }
    }
    if (it != this->end()) {
        delete it->second;
        this->erase(it);
    }
}
} // namespace SVParser
