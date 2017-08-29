#include "InputSequenceGenerator.hpp"
#include <log4cxx/logger.h>
#include <boost/format.hpp>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <random>
#include <ctime>
#include <map>
#include <memory>
#include <queue>
#include <sstream>
#include <string>

using log4cxx::LoggerPtr;
using log4cxx::Logger;
using boost::format;

extern std::ofstream output;
extern bool openSA;

namespace SVParser {

InputSequenceGenerator::InputSequenceGenerator()
    : _Base()
{
    ::yyparse(*this);
    Transition* trans = new Transition(Pattern(IPATTERNSIZE, 2), undefState, Pattern(OPATTERNSIZE, 2));
    undefState->transitions.push_back(trans);
    current = undefState;
    initial = getState(initialNumber);

    firstHalfAnswer.reserve(20);
}

InputSequenceGenerator::~InputSequenceGenerator()
{
    std::for_each(asrtList.begin(), asrtList.end(), std::default_delete< Assertion >());
}

void InputSequenceGenerator::preprocess()
{
    evalInitial2State();
    // purge unreach state
    for (auto it = this->begin(); it != this->end(); ++it)
        if (!it->second->traversed)
            purgeState(it->first);

    for (Assertion* asrt : asrtList) {
        staticFindActivatedPoint(*asrt);
        asrt->sortActivatedPointByLayer();
    }
}

void InputSequenceGenerator::assertionByOrder(std::vector< int >& order)
{
    std::sort(asrtList.begin(), asrtList.end(), [&order](const Assertion* lhs, const Assertion* rhs) {
        int a, b;
        sscanf(lhs->name.c_str(), "assertion_rule%d", &a);
        sscanf(rhs->name.c_str(), "assertion_rule%d", &b);
        return (std::find(order.begin(), order.end(), a) - order.begin()) < (std::find(order.begin(), order.end(), b) - order.begin());
    });
}

std::string name;

void InputSequenceGenerator::simulator()
{
    std::sort(asrtList.begin(), asrtList.end(), [](const Assertion* lhs, const Assertion* rhs) {
        return lhs->time.second > rhs->time.second;
    });
    for (Assertion* asrt : asrtList) {
        path.clear();
        asrtFailedFlag = false;
        asrt->noSolution = false;
        if (!asrt->failed)
            fromActivatedPoint2AssertionFailed(*asrt);
    }
    finalAnswer.clear();
    finalAnswer.push_back(InputPattern::random(IPATTERNSIZE));
    finalAnswer.push_back(InputPattern::random(IPATTERNSIZE).reset());
    generateSolution();
    if (openSA) {
        simulatedAnnealing();
    }
}

std::pair< size_t, size_t > rand2(size_t lower, size_t upper)
{
    assert(upper > lower);
    size_t range = upper - lower;
    int i1 = rand() % range + lower;
    int i2;
    while (i1 == (i2 = rand() % range + lower))
        ;
    return std::pair< size_t, size_t >(i1, i2);
}

void InputSequenceGenerator::simulatedAnnealing()
{
    static const LoggerPtr logger = Logger::getLogger("IGS.SA");
    static const std::default_random_engine randEng(std::random_device {}());
    static size_t tc = 0;
    const int M1_WEIGHT = 30;
    const int M2_WEIGHT = 30;
    const int M3_WEIGHT = 40;
    std::function< unsigned int() > select_op = std::bind(
        std::discrete_distribution<>({M1_WEIGHT, M2_WEIGHT, M3_WEIGHT}),
        randEng);

    std::function< float() > accept = std::bind(
        std::uniform_real_distribution<>(0, 1),
        randEng);
    float temperature = 100.0f;
    const float RATE = 0.95f;
    const int TIMES_PER_ROUND = 1000;
    InputSequence opt = finalAnswer;
    size_t localSize = finalAnswer.size();
    std::vector< Assertion* > optOrder;
    int r = 0;
    while (temperature > 1.0) {
        LOG4CXX_DEBUG(logger, ++tc << " try");

        finalAnswer.erase(finalAnswer.begin() + 2, finalAnswer.end());
        switch (select_op()) {
        case 0: {
            finalAnswer[0] = InputPattern::random(IPATTERNSIZE);
        } break;
        case 1: {
            finalAnswer[1] = InputPattern::random(IPATTERNSIZE).reset();
        } break;
        case 2: {
            // swap
            auto rs = rand2(0, asrtList.size());
            std::swap(asrtList[rs.first], asrtList[rs.second]);
        } break;
        default:
            std::exit(1);
            break;
        }

        // generate input sequence
        generateSolution();

        // accept
        if (localSize > finalAnswer.size()) {
            localSize = finalAnswer.size();
            if (opt.size() > finalAnswer.size()) {
                opt = finalAnswer;
                optOrder = asrtList;
                LOG4CXX_DEBUG(logger, "Optimal size update to " << opt.size());
            }
        } else {
            int s1 = finalAnswer.size(), s2 = localSize;
            int delta = abs(s1 - s2);
            float threshold = 1 / exp(delta / temperature);
            // condition accept
            if (accept() < threshold) {
                localSize = finalAnswer.size();
            }
        }
        if (!((r++) % TIMES_PER_ROUND))
            temperature *= RATE;
    }
    finalAnswer = opt;

    LOG4CXX_DEBUG(logger, "Optimal assertion order: ")
    std::for_each(optOrder.begin(), optOrder.end(), [](const Assertion* asrt) {
        LOG4CXX_DEBUG(logger, asrt->name);
    });
}

void InputSequenceGenerator::fromActivatedPoint2AssertionFailed(Assertion& asrt)
{
    bool signalFlag = asrt.event.target == TargetType::OUT ? true : false;
    bool triggerFlag = asrt.event.change == SignalEdge::ROSE ? true : false;
    unsigned int index = asrt.event.index;
    bool res = false;
    if (signalFlag) {
        for (auto ap = asrt.APList.begin(); ap != asrt.APList.end(); ++ap) {
            answerDict[&asrt].clear();
            asrt.arcIt = ap;
            res = fromActivatedPoint2AssertionOutputSignalFailed(asrt, answerDict[&asrt], ap->state, ap->transition1, ap->transition2, 0);
            if (res) {
                targetAP = *ap;
                break;
            }
        }
    }

    if (res) {
        answerDict[&asrt].front() = targetAP.pattern2.defaultPattern();
        initial2ActivatedArc();
        answerDict[&asrt].insert(answerDict[&asrt].begin(), firstHalfAnswer.rbegin(), firstHalfAnswer.rend());
    } else {
        asrt.noSolution = true;
    }
    firstHalfAnswer.clear();
}

bool InputSequenceGenerator::fromActivatedPoint2AssertionOutputSignalFailed(Assertion& asrt, InputSequence& sequence, State* current,
                                                                            Transition* t1, Transition* t2,
                                                                            size_t step)
{
    sequence.push_back(t2->defaultPattern());
    if (step > asrt.time.second) {
        return true;
    }
    if (step >= asrt.time.first) {
        bool income = false, outcome = false;
        size_t index = asrt.event.index;

        if (t1->out[index] == (asrt.event.change == SignalEdge::FELL ? 1 : 0)) {
            income = true;
        }
        if (t2->out[index] == (asrt.event.change == SignalEdge::FELL ? 0 : 1)) {
            outcome = true;
        }
        if (income && outcome) {
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
    sequence.pop_back();
    return false;
}

bool InputSequenceGenerator::fromActivatedPoint2AssertionOutputSignalFailed(Assertion& asrt, InputSequence& sequence, State* current,
                                                                            Transition* t1, Transition* t2,
                                                                            size_t step, size_t bound)
{
    static size_t counter = -1;
    if (counter == 0) {
        return false;
    }
    counter = bound != 0 ? bound : counter - 1;
    sequence.push_back(t2->defaultPattern());
    if (step > asrt.time.second) {
        return true;
    }
    if (step >= asrt.time.first) {
        bool income = false, outcome = false;
        size_t index = asrt.event.index;

        if (t1->out[index] == (asrt.event.change == SignalEdge::FELL ? 1 : 0)) {
            income = true;
        }
        if (t2->out[index] == (asrt.event.change == SignalEdge::FELL ? 0 : 1)) {
            outcome = true;
        }
        if (income && outcome) {
            sequence.pop_back();
            return false;
        }
    }
    current = t2->nState;
    for (auto trans = current->transitions.begin(); trans != current->transitions.end(); ++trans) {
        bool res = fromActivatedPoint2AssertionOutputSignalFailed(asrt, sequence, current, t2, *trans, step + 1, 0);
        if (res == true)
            return true;
    }
    sequence.pop_back();
    return false;
}

void InputSequenceGenerator::generateSolution()
{
    static const LoggerPtr logger = Logger::getLogger("IGS.genAnswer");
    const size_t ACT_BOUND = 10000;
    for (Assertion* asrt : asrtList) {
        asrt->failed = false;
    }
    LOG4CXX_DEBUG(logger, "<===== Start Generate =====>");
    upcomingAsrt = std::next(asrtList.begin());
    Assertion* holder = nullptr;
    current = initial;
    for (Assertion* asrt : asrtList) {
        careAsrt = asrt;
        if (!asrt->failed && !asrt->noSolution) {
            if (holder != asrt) {
                LOG4CXX_DEBUG(logger, format("%1% is picked at normal") % asrt->name);
                // try from current to default arc
                bool canFromCur2Arc = false;
                InputSequence seq1, seq2;
                size_t CUR_BOUND = asrt->APList.size() / 10;
                size_t curCounter = 0;
                for (auto arc = asrt->arcIt; arc != asrt->APList.end(); ++arc) {
                    seq1.clear(), seq2.clear();
                    if (++curCounter < CUR_BOUND)
                        break;
                    if (fromCurrent2Arc(*asrt, seq1, *arc)) {
                        if (fromActivatedPoint2AssertionOutputSignalFailed(*asrt, seq2, arc->state, arc->transition1, arc->transition2, 0)) {
                            if (seq1.size() + seq2.size() >= answerDict[asrt].size())
                                continue;
                            LOG4CXX_DEBUG(logger, "default length: " << answerDict[asrt].size() << " | current to failed length: " << seq1.size() + seq2.size());
                            LOG4CXX_DEBUG(logger, "current lenth: " << finalAnswer.size());
                            canFromCur2Arc = true;
                            finalAnswer.insert(finalAnswer.end(), seq1.rbegin(), seq1.rend());
                            seq2.front() = arc->pattern2.defaultPattern();
                            finalAnswer.insert(finalAnswer.end(), seq2.begin(), seq2.end());
                            break;
                        }
                    }
                }
                // can't from current to arc
                if (!canFromCur2Arc) {
                    if (finalAnswer.size() != 2)
                        finalAnswer.push_back(InputPattern(IPATTERNSIZE, 0, true));
                    finalAnswer.insert(finalAnswer.end(), answerDict[asrt].begin(), answerDict[asrt].end());
                }
            } else {
                holder = nullptr;
            }
        }
        assertionInspector(finalAnswer);
        if (upcomingAsrt != asrtList.end() && !(*upcomingAsrt)->failed && !(*upcomingAsrt)->noSolution) {
            int k = 0;
            for (AssertionStatus& as : triggeredAssertion) {
                if (as.target == *upcomingAsrt && !as.suc) {
                    LOG4CXX_DEBUG(logger, "check act " << ++k)
                    InputSequence seq;
                    if (!fromActivatedPoint2AssertionOutputSignalFailed(**upcomingAsrt, seq, current, as.trans1, as.trans2, as.slack, ACT_BOUND))
                        continue;
                    holder = *upcomingAsrt;
                    for (auto pit = std::next(seq.begin()); pit != seq.end(); ++pit) {
                        finalAnswer.push_back(*pit);
                    }
                    LOG4CXX_DEBUG(logger, format("%1% is picked at act") % as.target->name);
                    finalAnswer.insert(finalAnswer.end(), ++seq.begin(), seq.end());
                    LOG4CXX_DEBUG(logger, "gain " << (*upcomingAsrt)->name << ", " << seq.size() - 1 << " plus");
                    break;
                }
            }
        }
        triggeredAssertion.clear();
        ++upcomingAsrt;
    }
}

void InputSequenceGenerator::assertionInspector(InputSequence& seq)
{
    static const LoggerPtr logger = Logger::getLogger("IGS.inspector2");
    unsigned int time = 30;
    current = undefState;
    in2 = InputPattern(inputSize(), 2);
    out2 = Pattern(outputSize(), 2);
    auto breakIt = seq.end();
    int counter = 0;
    for (auto it = seq.begin(); it != seq.end(); ++it, time += 20) {
        ++counter;
        this->input(*it);
        LOG4CXX_TRACE(logger, format("%1%, %2% in state %|3$=10d| at %4%") % in2 % out2 % current->label % time);
        for (Assertion* asrt : asrtList) {
            if (asrt->failed)
                continue;
            size_t index = asrt->trigger.index;
            Pattern::value_type triggerFlag = (asrt->trigger.change == SignalEdge::ROSE) ? 1 : 0;
            bool signalFlag = (asrt->trigger.target == TargetType::OUT);

            Pattern& pre = (signalFlag ? out1 : in1),
                     &cur = (signalFlag ? out2 : in2);
            if (pre[index] != triggerFlag && cur[index] == triggerFlag) {
                //cout << "Trigger: " << asrt->name << " at " << counter << endl;
                triggeredAssertion.push_back(AssertionStatus{0, asrt, false});
            }
        }
        for (auto as = triggeredAssertion.begin(); as != triggeredAssertion.end(); ++as) {
            Assertion& asrt = *as->target;
            as->trans1 = trans1;
            as->trans2 = trans2;
            if (as->target->failed)
                continue;
            if (as->suc)
                continue;
            if (as->slack >= asrt.time.first && as->slack <= asrt.time.second) {
                size_t index = asrt.event.index;
                Pattern::value_type triggerFlag = (asrt.event.change == SignalEdge::ROSE) ? 1 : 0;
                bool signalFlag = (asrt.event.target == TargetType::OUT);
                Pattern& pre = (signalFlag ? out1 : in1),
                         &cur = (signalFlag ? out2 : in2);
                if (pre[index] != triggerFlag && cur[index] == triggerFlag) {
                    as->suc = true;
                    //cout << "Success: " << as->target->name << endl;
                }
            } else if (as->slack > asrt.time.second) {
                LOG4CXX_DEBUG(logger, format("%1% has been failed!") % asrt.name);
                asrt.failed = true;
                //cout << asrt.name << " failed at " << counter << endl;
                if (careAsrt->failed) {
                    breakIt = it;
                }
            }
            ++(as->slack);
        }
        if (breakIt != seq.end()) {
            seq.erase(++breakIt, seq.end());
            return;
        }
    }
}

InputPattern InputSequenceGenerator::evalSecondInput()
{
    Transition* selected = nullptr;
    int maxHit = 0;
    int index = 0;
    Pattern::value_type triggerFlag;
    for (Transition* trans : current->transitions) {
        int hit = 0;
        for (Assertion* asrt : asrtList) {
            if (asrt->trigger.target == TargetType::OUT)
                continue;
            triggerFlag = (asrt->event.change == SignalEdge::ROSE) ? 1 : 0;
            index = asrt->trigger.index;
            if (trans->out[index] == triggerFlag)
                ++hit;
        }
        if (hit >= maxHit)
            selected = trans, maxHit = hit;
    }
    return selected->defaultPattern();
}

InputPattern InputSequenceGenerator::evalStartInput()
{
    InputPattern input(inputSize());
    std::vector< int > counter(inputSize(), 0);
    for (Assertion* asrt : asrtList) {
        if (asrt->trigger.target == TargetType::OUT)
            continue;
        size_t index = asrt->trigger.index;
        counter[index] += (asrt->trigger.change == SignalEdge::ROSE) ? 1 : -1;
    }
    for (size_t index = 0; index < inputSize(); ++index) {
        input[index] = counter[index] > 0 ? 1 : 0;
    }
    return input;
}

void InputSequenceGenerator::evalInitial2State()
{
    std::queue< State* > bfsQueue;
    State* S0 = this->initial;
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

bool InputSequenceGenerator::fromCurrent2Arc(Assertion& asrt, InputSequence& seq, ActivatedPoint& arc)
{
    if (arc.state == initial)
        return false;
    resetTraversed();
    State* cur = nullptr;
    // setup the first state
    for (State::From& from : arc.state->fromList) {
        if (from.transition == arc.transition1) {
            cur = from.state;
            cur->traversed = true;
            seq.push_back(arc.pattern1.defaultPattern());
            // first back trace state is the current
            if (cur == current) {
                return true;
            }
            break;
        }
    }
    assert(cur != nullptr);
    while (!cur->deadEnd() && cur != initial) {
        // can't get shorter ans
        if (seq.size() >= asrt.arcIt->state->layer)
            return false;
        else {
        }
        for (State::From& from : cur->fromList) {
            // try to choose the trans from current
            if (from.state == current) {
                seq.push_back(from.transition->defaultPattern());
                return true;
            }
        }
        for (State::From& from : cur->fromList) {
            if (!from.state->traversed) {
                cur = from.state;
                cur->traversed = true;
                seq.push_back(from.transition->defaultPattern());
                break;
            }
        }
    }
    return false;
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
    firstHalfAnswer.push_back(targetAP.pattern1.defaultPattern());
    assert(current != nullptr);
    current->traversed = true;
    while (current->layer != 0) {
        for (State::From& from : current->fromList) {
            if (!from.state->traversed && (from.state->layer < current->layer)) {
                current = from.state;
                firstHalfAnswer.push_back(from.transition->defaultPattern());
                break;
            }
        }
    }
}

void InputSequenceGenerator::outputAnswer()
{
    for (auto pit = finalAnswer.begin(); pit != finalAnswer.end(); ++pit) {
        output << *pit << endl;
    }
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
