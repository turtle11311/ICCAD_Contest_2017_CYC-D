#include "FiniteStateMachine.hpp"
#include <iostream>
#include <limits>
#include <utility>
using std::endl;
using std::cout;

namespace SVParser {
FiniteStateMachine::FiniteStateMachine()
    : undefState(new State(std::numeric_limits< int >::max()))
{
}

void FiniteStateMachine::insesrtTransition(int state, Pattern&& pattern, int nState, Pattern&& out)
{
    State* nowState = getState(state), *nextState = getState(nState);
    nowState->transitions.push_back(
        new Transition(std::move(pattern), nextState, std::move(out)));

    nextState->fromList.push_back(
        State::From(nowState, nowState->transitions.back()));
}

State* FiniteStateMachine::getState(int state)
{
    std::map< int, State* >::iterator it;
    if ((it = this->find(state)) != this->end()) {
        return it->second;
    }
    return ((*this)[state] = new State(state));
}

void FiniteStateMachine::input(const InputPattern& pattern)
{
    in1 = in2;
    out1 = out2;
    in2 = pattern;
    if (pattern._reset)
        current = initial;
    for (Transition* trans : current->transitions) {
        if (trans->pattern == pattern) {
            current = trans->nState;
            out2 = trans->out;
            break;
        }
    }
    if (pattern._reset)
        current = initial;
}

FiniteStateMachine::~FiniteStateMachine()
{
    for (auto it = this->begin(); it != this->end(); ++it)
        delete it->second;
}

void FiniteStateMachine::setIsolatedState(int state)
{
    isolatedStates.insert((*this)[state]);
}

bool FiniteStateMachine::isIsolated(int state)
{
    return isolatedStates.end() == isolatedStates.find((*this)[state]);
}

void FiniteStateMachine::resetTraversed()
{
    for (auto& state : (*this)) {
        state.second->traversed = false;
    }
}
} // namespace SVParser
