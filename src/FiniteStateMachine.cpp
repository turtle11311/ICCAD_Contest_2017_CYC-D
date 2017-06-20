#include "FiniteStateMachine.hpp"
#include <iostream>
#include <utility>
using std::endl;
using std::cout;

namespace SVParser {
FiniteStateMachine::FiniteStateMachine()
{
    yyparse(*this);
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

void FiniteStateMachine::printStateLayer()
{
    for (auto it = this->begin(); it != this->end(); ++it) {
        cout << "S" << it->second->label << ": " << it->second->layer << (!it->second->traversed ? "X" : "") << endl;
    }
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
    for (int i = 0; i < this->size(); ++i) {
        (*this)[i]->traversed = false;
    }
}
}
