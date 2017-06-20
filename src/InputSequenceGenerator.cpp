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
