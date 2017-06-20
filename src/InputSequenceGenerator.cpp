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

void InputSequenceGenerator::purgeState(int state)
{
    auto it = this->find(state);
    if (it != this->end()) {
        delete it->second;
        this->erase(it);
    }
}
}
