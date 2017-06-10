#pragma once
#include "State.hpp"
#include <algorithm>
#include <cstdio>
#include <map>

namespace SVParser {
class FiniteStateMachine {
public:
    FiniteStateMachine();
    void insesrtTransition(int state, Pattern&& pattern, int nState, Pattern&& out);
    void _Debug_show_all_data();
    ~FiniteStateMachine();

private:
    State* getState(int state);
    std::map< int, State* > stateTable;
    size_t PATTERNSIZE;
};
}