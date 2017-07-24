#include "InputSequenceGenerator.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <ctime>
#include <map>
#include <queue>
#include <sstream>
#include <string>
extern std::ofstream output;
extern bool separableMode;
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

void InputSequenceGenerator::assertionByOrder(std::vector< int >& order)
{
    asrtList.sort([&order](const Assertion& lhs, const Assertion& rhs) {
            int a, b;
            sscanf(lhs.name.c_str(), "assertion_rule%d", &a);
            sscanf(rhs.name.c_str(), "assertion_rule%d", &b);
            return (std::find(order.begin(), order.end(), a) - order.begin()) <
                   (std::find(order.begin(), order.end(), b) - order.begin());
    });
}

std::string name;

void InputSequenceGenerator::simulator()
{
    cout << "Simulator!\n";
    asrtList.sort([](const Assertion& lhs, const Assertion& rhs) {
        return lhs.time.second > rhs.time.second;
    });
    for (Assertion& asrt : asrtList) {
        // cout << asrt.name << ": " << endl;
        path.clear();
        asrtFailedFlag = false;
        asrt.noSolution = false;
        if (!asrt.failed)
            fromActivatedPoint2AssertionFailed(asrt);
        // cout << endl
        //      << endl;
    }
    for (Assertion& asrt : asrtList) {
        InputSequence& answer = answerDict[&asrt];
        if (answer.size() == 0)
            continue;
        std::ofstream file(asrt.name + ".txt");
        file << 0 << Pattern(PATTERNSIZE) << endl;
        file << 1 << Pattern(PATTERNSIZE) << endl;
        for (auto iit = answer.begin(); iit != answer.end(); ++iit) {
            file << 0 << *iit << endl;
        }
        file.close();
    }
    generateSolution();
    //if (!separableMode)
    //simulatedAnnealing();
}

void InputSequenceGenerator::generateSolution()
{
    for (Assertion& asrt : asrtList) {
        asrt.failed = false;
    }
    finalAnswer.clear();
    rstTable.clear();
    // if (answerDict[&asrtList.front()].size() != 0) {
    //     rstTable.push_back(finalAnswer.size() + answerDict[&asrtList.front()].size() + 1);
    //     for (auto pit = answerDict[&asrtList.front()].begin(); pit != answerDict[&asrtList.front()].end(); ++pit)
    //         finalAnswer.push_back(*pit);
    // }
    for (Assertion& asrt : asrtList) {
        cout << asrt.name << " " << answerDict[&asrt].size() << endl;
        if (asrt.failed || asrt.noSolution)
            continue;
        cout << "pick." << endl;
        rstTable.push_back(finalAnswer.size() + answerDict[&asrt].size() + 1);
        for (auto pit = answerDict[&asrt].begin(); pit != answerDict[&asrt].end(); ++pit)
            finalAnswer.push_back(*pit);
        assertionInspector(finalAnswer);
    }
    cout << "Size: " << finalAnswer.size() << endl;
}

void InputSequenceGenerator::randomSwap4SA(int i1, int i2)
{
    int i = 0;
    std::list< Assertion >::iterator it1, it2;
    for (auto it = asrtList.begin(); it != asrtList.end(); ++it) {
        if (i1 == i)
            it1 = it;
        else if (i2 == i)
            it2 = it;
        ++i;
    }
    std::swap(*it1, *it2);
    std::swap(answerDict[&(*it1)], answerDict[&(*it2)]);
}

void InputSequenceGenerator::simulatedAnnealing()
{
    float temperature = 100.0f;
    float rate = 0.95f;
    InputSequence opt = finalAnswer;
    std::list< int > optRstTable = rstTable;
    InputSequence local = finalAnswer;
    std::list< std::string > optOrder;

    int r = 0;
    while (temperature > 1.0) {
        cout << "Local optimal length: " << local.size() << endl;

        // swap
        int i1 = rand() % asrtList.size();
        int i2;
        while (i1 == (i2 = rand() % asrtList.size()))
            ;
        randomSwap4SA(i1, i2);

        std::list< std::string > curOrder;
        // generate input sequence
        generateSolution();

        // accept
        cout << "Current length: " << finalAnswer.size() << endl;
        if (local.size() > finalAnswer.size()) {
            local = finalAnswer;
            if (opt.size() > finalAnswer.size()) {
                opt = finalAnswer;
                optOrder = curOrder;
                optRstTable = rstTable;
            }
        } else {
            int s1 = finalAnswer.size(), s2 = local.size();
            int delta = abs(s1 - s2);
            float threshold = 1 / exp(delta / temperature);
            // condition accept
            if (((float)(rand() % 1000)) / 1000 < threshold) {
                local = finalAnswer;
            } else {
                randomSwap4SA(i1, i2);
            }
        }
        if (!((r++) % 10))
            temperature *= rate;
    }
    cout << "Optimal length: " << opt.size() << endl;
    finalAnswer = opt;
    rstTable = optRstTable;
    std::ofstream oo("opt.order");
    for (auto od : optOrder) {
        oo << od << endl;
    }
    oo.close();
}

std::string tmps;

std::stringstream ss;
std::list< std::string > PATH;

void InputSequenceGenerator::fromActivatedPoint2AssertionFailed(Assertion& asrt)
{
    bool signalFlag = asrt.event.target == TargetType::OUT ? true : false;
    bool triggerFlag = asrt.event.change == SignalEdge::ROSE ? true : false;
    unsigned int index = asrt.event.index;
    // cout << "Activated target: " << ((asrt.trigger.target == TargetType::OUT) ? "out[" : "in[")
    //      << asrt.trigger.index << "]"
    //      << " is " << (asrt.trigger.change == SignalEdge::ROSE ? "rose" : "fell")
    //      << endl;
    // cout << "Terminate target: " << ((signalFlag) ? "out[" : "in[") << index << "]"
    //      << " is " << (triggerFlag ? "rose" : "fell")
    //      << " in range[" << asrt.time.first << ":" << asrt.time.second << "]"
    //      << "." << endl;
    bool res = false;
    if (signalFlag) {
        for (ActivatedPoint& ap : asrt.APList) {
            ss.clear();
            ss.str("");
            PATH.clear();
            answerDict[&asrt].clear();
            res = fromActivatedPoint2AssertionOutputSignalFailed(asrt, answerDict[&asrt], ap.state, ap.transition1, ap.transition2, 1);
            if (res) {
                // for (auto it = PATH.begin(); it != PATH.end(); ++it) {
                //     cout << *it << endl;
                // }
                targetAP = ap;
                // targetAP.printAP();
                break;
            }
        }
    }
    // cout << (res ? asrt.name + " Fail" : " QAQ") << endl;

    if (res) {
        answerDict[&asrt].pop_front();
        answerDict[&asrt].push_front(targetAP.pattern2.defaultPattern());
        // cout << "=====================================" << endl;
        initial2ActivatedArc();
        for (auto pit = firstHalfAnswer.rbegin(); pit != firstHalfAnswer.rend(); ++pit) {
            answerDict[&asrt].push_front(*pit);
        }
        // if (separableMode) {
        //     asrt.failed = false;
        //
        //     coverage.open(name + ".coverage");
        //     act.open(name + ".act");
        //
        //     assertionInspector(answerDict[&asrt]);
        //
        //     coverage.close();
        //     act.close();
        // } else {
        //     // rstTable.push_back(finalAnswer.size() + answerDict[&asrt].size() + 1);
        //     // for (auto pit = answerDict[&asrt].begin(); pit != answerDict[&asrt].end(); ++pit)
        //     //     finalAnswer.push_back(*pit);
        //     // assertionInspector(finalAnswer);
        // }
    } else {
        asrt.noSolution = true;
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

void InputSequenceGenerator::assertionInspector(InputSequence& seq)
{
    current = getState(0);
    std::ofstream ff(name + ".out");
    ff << endl
       << endl;
    in2 = Pattern(inputSize());
    out2 = Pattern(inputSize());
    int tc = 2;
    int index = 0;
    std::list< int > rstTemp = rstTable;
    for (auto it = seq.begin(); it != seq.end(); ++it) {
        index++;
        if (!separableMode) {
            if (rstTemp.front() == index) {
                rstTemp.pop_front();
                current = getState(0);
            }
        }
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
                triggeredAssertion.push_back(AssertionStatus{0, &asrt, false});
            }
        }
        for (auto as = triggeredAssertion.begin(); as != triggeredAssertion.end(); ++as) {
            Assertion& asrt = *as->target;
            if (as->target->failed)
                continue;
            if (as->suc)
                continue;
            if (as->slack >= asrt.time.first && as->slack < asrt.time.second) {
                size_t index = asrt.event.index;
                bool triggerFlag = (asrt.event.change == SignalEdge::ROSE);
                bool signalFlag = (asrt.event.target == TargetType::OUT);
                Pattern pre = (signalFlag ? out1 : in1),
                        cur = (signalFlag ? out2 : in2);
                if (pre[index] == !triggerFlag && cur[index] == triggerFlag) {
                    as->suc = true;
                }
            } else if (as->slack >= asrt.time.second) {
                if (separableMode)
                    coverage << asrt.name << endl;
                else {
                    asrt.failed = true;
                }
            }
            ++(as->slack);
        }
        ++tc;
    }

    if (separableMode) {
        for (auto as = triggeredAssertion.begin(); as != triggeredAssertion.end(); ++as) {
            Assertion& asrt = *as->target;
            if (as->target->failed)
                continue;
            if (as->suc)
                continue;
            act << asrt.name << endl;
        }
    }
    ff.close();
    triggeredAssertion.clear();
}

Pattern InputSequenceGenerator::evalStartInput()
{
    Pattern input(inputSize());
    std::vector< int > counter(inputSize(), 0);
    for (Assertion& asrt : asrtList) {
        if (asrt.trigger.target == TargetType::OUT)
            continue;
        size_t index = asrt.trigger.index;
        counter[index] += (asrt.trigger.change == SignalEdge::ROSE) ? 1 : -1;
    }
    for (size_t index = 0; index < inputSize(); ++index) {
        input[index] = counter[index] > 0 ? 1 : 0;
    }
    return input;
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
    // cout << "Layer: " << current->layer << " " << current->label << " " << *targetAP.transition1 << endl;
    firstHalfAnswer.push_front(targetAP.pattern1.defaultPattern());
    assert(current != nullptr);
    current->traversed = true;
    while (current->layer != 0) {
        for (State::From& from : current->fromList) {
            if (!from.state->traversed && (from.state->layer < current->layer)) {
                current = from.state;
                // cout << "Layer: " << current->layer << " " << current->label << " " << *from.transition << endl;
                firstHalfAnswer.push_front(from.transition->defaultPattern());
                break;
            }
        }
    }
}

void InputSequenceGenerator::outputAnswer()
{
    if (separableMode) {
        for (Assertion& asrt : asrtList) {
            InputSequence& answer = answerDict[&asrt];
            if (answer.size() == 0)
                continue;
            std::ofstream file(asrt.name + ".txt");
            file << 0 << Pattern(PATTERNSIZE) << endl;
            file << 1 << Pattern(PATTERNSIZE) << endl;
            for (auto iit = answer.begin(); iit != answer.end(); ++iit) {
                file << 0 << *iit << endl;
            }
            file.close();
        }
    } else {
        output << 0 << evalStartInput() << endl;
        output << 1 << Pattern(PATTERNSIZE) << endl;
        std::list< int > rstTemp = rstTable;
        int index = 0;
        for (auto pit = finalAnswer.begin(); pit != finalAnswer.end(); ++pit) {
            ++index;
            if (rstTemp.front() == index) {
                output << 1 << Pattern(PATTERNSIZE) << endl;
                rstTemp.pop_front();
            }
            output << 0 << *pit << endl;
        }
    }
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
