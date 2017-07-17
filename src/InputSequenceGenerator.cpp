#include "InputSequenceGenerator.hpp"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <map>
#include <queue>
#include <sstream>
extern std::ofstream output;
namespace SVParser {

InputSequenceGenerator::InputSequenceGenerator()
    : _Base()
{
    ::yyparse(*this);
    current = getState(0);
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
    asrtList.sort([](const Assertion& lhs, const Assertion& rhs) {
        return lhs.time.second > rhs.time.second;
    });
    for (Assertion& asrt : asrtList) {
        cout << asrt.name << ": " << endl;
        path.clear();
        asrtFailedFlag = false;
        if (!asrt.failed)
            fromActivatedPoint2AssertionFailed(asrt);
        cout << endl
             << endl;
    }
}

std::string name;
std::string tmps;

std::stringstream ss;
std::list< std::string > PATH;

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
            ss.clear();
            ss.str("");
            PATH.clear();
            answerDict[&asrt].clear();
            res = fromActivatedPoint2AssertionOutputSignalFailed(asrt, answerDict[&asrt], ap.state, ap.transition1, ap.transition2, 1);
            if (res) {
                for (auto it = PATH.begin(); it != PATH.end(); ++it) {
                    cout << *it << endl;
                }
                targetAP = ap;
                targetAP.printAP();
                break;
            }
        }
    }
    cout << (res ? asrt.name + " Fail" : " QAQ") << endl;

    if (res) {
        asrt.failed = true;
        answerDict[&asrt].pop_front();
        answerDict[&asrt].push_front(targetAP.pattern2.defaultPattern());
        cout << "=====================================" << endl;
        initial2ActivatedArc();
        for (auto pit = firstHalfAnswer.rbegin(); pit != firstHalfAnswer.rend(); ++pit) {
            answerDict[&asrt].push_front(*pit);
        }
        name = asrt.name;
        assertionInspector(answerDict[&asrt]);
    }
    firstHalfAnswer.clear();
}

bool InputSequenceGenerator::fromActivatedPoint2AssertionOutputSignalFailed(Assertion& asrt, InputSequence& sequence, State* current, Transition* t1, Transition* t2, size_t step)
{
    ss << current->label << " " << *t2 << endl;
    getline(ss, tmps);
    PATH.emplace_back(tmps);
    sequence.push_back(t2->defaultPattern());
    if (step > asrt.time.second) {
        asrt.failed = true;
        return true;
    }
    if (step > asrt.time.first) {
        bool income = false, outcome = false;
        size_t index = asrt.event.index;

        if (t1->out[index] == (asrt.event.change == SignalEdge::FELL ? 1 : 0)) {
            income = true;
        }
        if (t2->out[index] == (asrt.event.change == SignalEdge::FELL ? 0 : 1)) {
            outcome = true;
        }
        if (income && outcome) {
            PATH.pop_back();
            sequence.pop_back();
            return false;
        }
    }
    current = t2->nState;
    for (auto trans = current->transitions.begin(); trans != current->transitions.end(); ++trans) {
        bool res = fromActivatedPoint2AssertionOutputSignalFailed(asrt, sequence, current, t2, *trans, step + 1);
        if (res == true)
            return true;
    }
    PATH.pop_back();
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

void InputSequenceGenerator::assertionInspector(InputSequence& seq)
{
    current = getState(0);
    std::ofstream ff(name + ".out");
    std::ofstream coverage(name + ".coverage");
    ff << endl
       << endl;
    in2 = Pattern(inputSize());
    out2 = Pattern(inputSize());
    int tc = 2;
    for (auto it = seq.begin(); it != seq.end(); ++it) {
        this->input(*it);
        ff << out2 << endl;
        for (Assertion& asrt : asrtList) {
            if (asrt.failed)
                continue;
            size_t index = asrt.trigger.index;
            bool triggerFlag = (asrt.trigger.change == SignalEdge::ROSE);
            bool signalFlag = (asrt.trigger.target == TargetType::OUT);

            Pattern pre = (signalFlag ? out1 : in1),
                    cur = (signalFlag ? out2 : in2);
            if (pre[index] == !triggerFlag && cur[index] == triggerFlag) {
                triggeredAssertion.push_back(AssertionStatus{ 0, &asrt, false });
            }
        }
        for (auto as = triggeredAssertion.begin(); as != triggeredAssertion.end(); ++as) {
            Assertion& asrt = *as->target;
            if (as->target->failed)
                continue;
            if (as->suc)
                continue;
            if (as->slack > asrt.time.second) {
                cout << asrt.name << " Fail!!!" << endl;
                cout << asrt.time.second << " " << as->slack << endl;
                //asrt.failed = true;
                coverage << asrt.name << endl;
            } else if (as->slack >= asrt.time.first) {
                size_t index = asrt.event.index;
                bool triggerFlag = (asrt.event.change == SignalEdge::ROSE);
                bool signalFlag = (asrt.event.target == TargetType::OUT);
                Pattern pre = (signalFlag ? out1 : in1),
                        cur = (signalFlag ? out2 : in2);
                if (pre[index] == !triggerFlag && cur[index] == triggerFlag) {
                    as->suc = true;
                }
            }
            ++(as->slack);
        }
        ++tc;
    }
    triggeredAssertion.clear();
    ff.close();
    coverage.close();
}

void InputSequenceGenerator::findInputSignalTermiateStartPoint(bool triggerFlag, unsigned int index, ActivatedPoint& ap, Range& range)
{
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
                        APList.push_back(ActivatedPoint(trans1->nState, trans1->pattern, trans2->pattern, trans1, trans2));
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
                        APList.push_back(ActivatedPoint(trans1->nState, expectedPattern1, expectedPattern2, trans1, trans2));
                    }
                }
            }
        }
    }
}

void InputSequenceGenerator::initial2ActivatedArc()
{
    firstHalfAnswer.clear();
    resetTraversed();
    State* current = nullptr;
    for (State::From& from : targetAP.state->fromList) {
        if (from.transition == targetAP.transition1) {
            current = from.state;
            break;
        }
    }
    cout << "Layer: " << current->layer << " " << current->label << " " << *targetAP.transition1 << endl;
    firstHalfAnswer.push_front(targetAP.pattern1.defaultPattern());
    assert(current != nullptr);
    current->traversed = true;
    while (current->layer != 0) {
        for (State::From& from : current->fromList) {
            if (!from.state->traversed && (from.state->layer < current->layer)) {
                current = from.state;
                cout << "Layer: " << current->layer << " " << current->label << " " << *from.transition << endl;
                firstHalfAnswer.push_front(from.transition->defaultPattern());

                break;
            }
        }
    }
    if (current->label == 0)
        cout << "WTF\n";
}

void InputSequenceGenerator::outputAnswer()
{
    for (Assertion& asrt : asrtList) {
        InputSequence& answer = answerDict[&asrt];
        if (answer.size() == 0)
            continue;
        cout << asrt.name << endl;
        std::ofstream file(asrt.name + ".txt");
        output << 0 << Pattern(PATTERNSIZE) << endl;
        output << 1 << Pattern(PATTERNSIZE) << endl;
        file << 0 << Pattern(PATTERNSIZE) << endl;
        file << 1 << Pattern(PATTERNSIZE) << endl;
        for (auto iit = answer.begin(); iit != answer.end(); ++iit) {
            output << 0 << *iit << endl;
            file << 0 << *iit << endl;
        }
        file.close();
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
