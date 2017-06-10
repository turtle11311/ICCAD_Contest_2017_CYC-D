#include "FiniteStateMachine.hpp"
#include <iostream>
#include <utility>

namespace SVParser {
FiniteStateMachine::FiniteStateMachine()
{
}

void FiniteStateMachine::insesrtTransition(int state, Pattern&& pattern, int nState, Pattern&& out)
{
    PATTERNSIZE = pattern.size();
    State *nowState = getState(state), *nextState = getState(nState);
    nowState->transitions.push_back(
        new Transition(std::move(pattern), nextState, std::move(out)));

    nextState->fromList.push_back(std::move(
        State::From(nowState, nowState->transitions.back())));
}

State* FiniteStateMachine::getState(int state)
{
    std::map< int, State* >::iterator it;
    if ((it = this->find(state)) != this->end()) {
        return it->second;
    }
    return ((*this)[state] = new State(state));
}

FiniteStateMachine::~FiniteStateMachine()
{
    for (auto it = this->begin(); it != this->end(); ++it)
        delete it->second;
}
}