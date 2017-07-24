#pragma once
#include "State.hpp"
#include <algorithm>
#include <cstdio>
#include <map>
#include <set>

namespace SVParser {
class FiniteStateMachine : protected std::map< int, State* > {
    typedef std::map< int, State* > _Base;

public:
    using _Base::operator[];
    using _Base::size;
    using _Base::begin;
    using _Base::end;
    FiniteStateMachine();
    inline size_t inputSize()
    {
        return IPATTERNSIZE;
    }
    inline size_t outputSize()
    {
        return OPATTERNSIZE;
    }
    void input(const Pattern& pattern);
    State* getState(int state);
    void printStateLayer();
    void setIsolatedState(int state);
    void resetTraversed();
    bool isIsolated(int state);
    ~FiniteStateMachine();

protected:
    void insesrtTransition(int state, Pattern&& pattern, int nState, Pattern&& out);
    size_t IPATTERNSIZE;
    size_t OPATTERNSIZE;
    std::set< State* > isolatedStates;
    std::map< int, std::vector< State* > > rlayerTable;
    State* current;
    Pattern out1, out2;
    Pattern in1, in2;
};
} // namespace SVParser
