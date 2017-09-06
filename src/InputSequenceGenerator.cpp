#include "InputSequenceGenerator.hpp"
#include <algorithm>
#include <boost/format.hpp>
#include <cassert>
#include <cmath>
#include <ctime>
#include <log4cxx/logger.h>
#include <map>
#include <memory>
#include <queue>
#include <random>
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

void InputSequenceGenerator::assertionByOrder(std::initializer_list< int > order)
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
    //assertionByOrder({9,6,1,8,7,5,10,4,3,2});
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
    //finalAnswer.push_back(InputPattern("0101100"));
    //finalAnswer.push_back(InputPattern("1000011").reset());
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
    static const std::default_random_engine randEng(std::random_device{}());
    static size_t tc = 0;
    const int M1_WEIGHT = 30;
    const int M2_WEIGHT = 30;
    const int M3_WEIGHT = 10;
    const int M4_WEIGHT = 30;
    std::function< unsigned int() > select_op = std::bind(
        std::discrete_distribution<>({M1_WEIGHT, M2_WEIGHT, M3_WEIGHT, M4_WEIGHT}),
        randEng);

    std::function< float() > accept = std::bind(
        std::uniform_real_distribution<>(0, 1),
        randEng);
    float temperature = 100.0f;
    const float RATE = 0.95f;
    const int TIMES_PER_ROUND = 2000;
    InputSequence opt = finalAnswer;
    size_t localSize = finalAnswer.size();
    std::vector< Assertion* > optOrder;
    int r = 0;
    size_t top1 = 0;
    while (temperature > 1.0) {
        LOG4CXX_DEBUG(logger, ++tc << " try");

        finalAnswer.erase(finalAnswer.begin() + 2, finalAnswer.end());
        auto rs = rand2(0, asrtList.size());
        int move = select_op();
        switch (move) {
        case 0: {
            finalAnswer[0] = InputPattern::random(IPATTERNSIZE);
        } break;
        case 1: {
            finalAnswer[1] = InputPattern::random(IPATTERNSIZE).reset();
        } break;
        case 2: {
            for (auto& state : *this) {
                std::random_shuffle(state.second->transitions.begin(), state.second->transitions.end());
            }
            for (Assertion* asrt : asrtList) {
                path.clear();
                asrtFailedFlag = false;
                asrt->noSolution = false;
                if (!asrt->failed)
                    fromActivatedPoint2AssertionFailed(*asrt);
            }
        } break;
        case 3: {
            // swap
            std::swap(asrtList[rs.first], asrtList[rs.second]);
        } break;
        default:
            std::exit(1);
            break;
        }
        // generate input sequence
        generateSolution();

        LOG4CXX_DEBUG(logger, "Now length:" << finalAnswer.size());
        // accept

        if (top1 < failedAssertion.size()) {
            localSize = finalAnswer.size();
            if (opt.size() > finalAnswer.size()) {
                r = 0;
                opt = finalAnswer;
                optOrder = asrtList;
                top1 = failedAssertion.size();
                LOG4CXX_DEBUG(logger, "Optimal size update to " << opt.size());
            }
        }
        else if (top1 == failedAssertion.size()) {
            if (localSize > finalAnswer.size()) {
                localSize = finalAnswer.size();
                if (opt.size() > finalAnswer.size()) {
                    r = 0;
                    opt = finalAnswer;
                    optOrder = asrtList;
                    LOG4CXX_DEBUG(logger, "Optimal size update to " << opt.size());
                }
            }
            else {
                int s1 = finalAnswer.size(), s2 = localSize;
                int delta = abs(s1 - s2);
                float threshold = 1 / exp(delta / temperature);
                // condition accept
                if (accept() < threshold) {
                    localSize = finalAnswer.size();
                }
                else {
                    if (move == 3)
                        std::swap(asrtList[rs.first], asrtList[rs.second]);
                }
            }
        }
        if (!((r++) % TIMES_PER_ROUND))
            temperature *= RATE;
    }
    finalAnswer = opt;

    LOG4CXX_DEBUG(logger, "Optimal assertion order: ");
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
    for (auto ap = asrt.APList.begin(); ap != asrt.APList.end(); ++ap) {
        answerDict[&asrt].clear();
        asrt.arcIt = ap;
        if (signalFlag) {
            res = fromActivatedPoint2AssertionOutputSignalFailed(asrt, answerDict[&asrt], ap->state, ap->transition1, ap->transition2, 0);
        }
        else {
            res = fromActivatedPoint2AssertionInputSignalFailed(asrt, answerDict[&asrt]);
        }
        if (res) {
            targetAP = *ap;
            break;
        }
    }

    if (res) {
        answerDict[&asrt].front() = targetAP.pattern2.defaultPattern();
        initial2ActivatedArc();
        answerDict[&asrt].insert(answerDict[&asrt].begin(), firstHalfAnswer.rbegin(), firstHalfAnswer.rend());
    }
    else {
        asrt.noSolution = true;
    }
    firstHalfAnswer.clear();
}

bool InputSequenceGenerator::fromActivatedPoint2AssertionInputSignalFailed(Assertion& asrt, InputSequence& sequence)
{
    sequence.insert(sequence.end(), asrt.time.second, InputPattern(IPATTERNSIZE, 0, false));
    return true;
}

bool InputSequenceGenerator::fromActivatedPoint2AssertionOutputSignalFailed(Assertion& asrt, InputSequence& sequence, State* current,
                                                                            Transition* t1, Transition* t2,
                                                                            size_t step)
{
    if (sequence.size() == 0)
        sequence.push_back(t2->defaultPattern());
    else
        sequence.push_back(t2->pattern.flipBy(sequence.back()));

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
    if (sequence.size() == 0)
        sequence.push_back(t2->defaultPattern());
    else
        sequence.push_back(t2->pattern.flipBy(sequence.back()));

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
    failedAssertion.clear();
    for (Assertion* asrt : asrtList) {
        asrt->failed = false;
        if (asrt->noSolution)
            failedAssertion.insert(asrt);
    }
    LOG4CXX_DEBUG(logger, "<===== Start Generate =====>");
    upcomingAsrt = std::next(asrtList.begin());
    Assertion* holder = nullptr;
    current = initial;
    LOG4CXX_DEBUG(logger, "+++++++++++++++++++++++++++++++++");
    for (Assertion* asrt : asrtList) {
        careAsrt = asrt;
        LOG4CXX_DEBUG(logger, asrt->name);
        if (!asrt->failed && !asrt->noSolution) {
            if (holder != asrt) {
                LOG4CXX_DEBUG(logger, format("%1% is picked at normal") % asrt->name);
                if (finalAnswer.size() != 2)
                    finalAnswer.push_back(InputPattern(IPATTERNSIZE, 0, true));
                finalAnswer.insert(finalAnswer.end(), answerDict[asrt].begin(), answerDict[asrt].end());
                LOG4CXX_DEBUG(logger, "Pick");
            }
            else {
                LOG4CXX_DEBUG(logger, "No pick");
                holder = nullptr;
            }
        }
        assertionInspector(finalAnswer);
        if (!asrt->failed && !asrt->noSolution) {
            LOG4CXX_DEBUG(logger, "Pick Failed!!");
        }

        if (upcomingAsrt != asrtList.end() && !(*upcomingAsrt)->failed && !(*upcomingAsrt)->noSolution) {
            int k = 0;
            for (AssertionStatus& as : triggeredAssertion) {
                if (as.target == *upcomingAsrt && !as.suc) {
                    LOG4CXX_DEBUG(logger, "check act " << ++k)
                    InputSequence seq;
                    // cout << as.slack - 1 << endl;
                    for (auto trans = current->transitions.begin(); trans != current->transitions.end(); ++trans) {
                        bool res = fromActivatedPoint2AssertionOutputSignalFailed(**upcomingAsrt, seq, current, trans2, *trans, as.slack, ACT_BOUND);
                        if (res == true)
                            break;
                    }
                    holder = *upcomingAsrt;
                    LOG4CXX_DEBUG(logger, format("%1% is picked at act length %2%") % as.target->name % seq.size());
                    finalAnswer.insert(finalAnswer.end(), seq.begin(), seq.end());
                    LOG4CXX_DEBUG(logger, "gain " << (*upcomingAsrt)->name << ", " << seq.size() - 1 << " plus");
                    break;
                }
            }
        }
        triggeredAssertion.clear();
        ++upcomingAsrt;
    }
    if (failedAssertion.size() != asrtList.size())
        LOG4CXX_DEBUG(logger, "Bad Solution");
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
            if (asrt->failed || asrt->noSolution)
                continue;
            size_t index = asrt->trigger.index;
            Pattern::value_type triggerFlag = (asrt->trigger.change == SignalEdge::ROSE) ? 1 : 0;
            bool signalFlag = (asrt->trigger.target == TargetType::OUT);

            Pattern &pre = (signalFlag ? out1 : in1),
                    &cur = (signalFlag ? out2 : in2);
            if (pre[index] != triggerFlag && cur[index] == triggerFlag) {
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
                Pattern &pre = (signalFlag ? out1 : in1),
                        &cur = (signalFlag ? out2 : in2);
                if (pre[index] != triggerFlag && cur[index] == triggerFlag) {
                    as->suc = true;
                }
                else if (as->slack == asrt.time.second) {
                    LOG4CXX_DEBUG(logger, format("%1% has been failed!") % asrt.name);
                    asrt.failed = true;
                    failedAssertion.insert(as->target);
                    if (careAsrt->failed) {
                        if (it->_reset)
                            seq.erase(std::next(it), seq.end());
                    }
                }
            }
            ++(as->slack);
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

void InputSequenceGenerator::evalInputSequence(std::string filename)
{
    std::ifstream file(filename, std::ios::in);
    char line[200];
    InputSequence sequence;
    sequence.reserve(200);
    while (file.getline(line, 200))
        sequence.push_back(InputPattern(line + 1, line[0] - '0'));
    assertionInspector(sequence);
}

} // namespace SVParser
