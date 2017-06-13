#pragma once
#include "State.hpp"
#include <algorithm>
#include <cstdio>
#include <map>

namespace SVParser {
class FiniteStateMachine : private std::map< int, State* > {
    typedef std::map< int, State* > _Base;

public:
    using _Base::operator[];
    using _Base::_Base;
    using _Base::size;
    using _Base::begin;
    using _Base::end;
    FiniteStateMachine();
    inline size_t inputSize() { return PATTERNSIZE; }
    State* getState(int state);
    void insesrtTransition(int state, Pattern&& pattern, int nState, Pattern&& out);
    void printStateLayer();
    ~FiniteStateMachine();

private:
    size_t PATTERNSIZE;
};
}
