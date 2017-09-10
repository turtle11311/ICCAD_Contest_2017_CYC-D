#include "FiniteStateMachine.hpp"
#include <log4cxx/logger.h>
#include <iostream>
#include <boost/format.hpp>
#include <limits>
#include <utility>
using std::endl;
using std::cout;
using boost::format;
using log4cxx::LoggerPtr;
using log4cxx::Logger;

namespace SVParser {
FiniteStateMachine::FiniteStateMachine()
    : undefState(new State(std::numeric_limits< int >::max()))
{
}

void FiniteStateMachine::insesrtTransition(int state, Pattern&& pattern, int nState, Pattern&& out)
{
    static const LoggerPtr logger = Logger::getLogger("preprocess.insertTransition");
    State* nowState = getState(state),
           *nextState = getState(nState);

    std::vector< Transition* >& transList = nowState->transitions;

    // check coverage
    for (size_t i = 0; i < transList.size(); ++i) {
        Pattern& select = transList[i]->pattern;
        Pattern inter = select.intersectionWith(pattern);
        if (inter == pattern) {
            LOG4CXX_DEBUG(logger, i);
            LOG4CXX_DEBUG(logger, format("%1% state: %2% => %3% go %4% is redundent") % state % pattern % out % nState);
            return;
        } else if (!inter.empty()) {
            LOG4CXX_DEBUG(logger, "<==================================>");
            LOG4CXX_DEBUG(logger, format("%1% state: %2% => %3% go %4%") % state % pattern % out % nState);
            pattern = pattern.diffWith(inter);
            LOG4CXX_DEBUG(logger, format("%1% state: %2% => %3% go %4%") % state % pattern % out % nState);
        }
    }

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
    trans1 = trans2;
    in2 = pattern;
    if (pattern._reset) {
        current = initial;
        preCur = current;
    }
    for (Transition* trans : current->transitions) {
        if (trans->pattern == pattern) {
            preCur = current;
            current = trans->nState;
            out2 = trans->out;
            trans2 = trans;
            break;
        }
    }
    if (pattern._reset) {
        current = initial;
    }
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
