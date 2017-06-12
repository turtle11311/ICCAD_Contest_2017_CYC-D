#pragma once
#include "FiniteStateMachine.hpp"
#include "Pattern.hpp"
#include <cstdlib>
#include <utility>
#include "State.hpp"
#include <list>

namespace SVParser {

struct ActivatedPoint{
    State* state;
    Pattern pattern1, pattern2;
    Transition *transition1, *transition2;
};

class Range : public std::pair< size_t, size_t > {
    typedef std::pair< size_t, size_t > _Base;

public:
    Range(size_t pos = 0)
        : _Base(pos, pos)
    {
    }
    Range(size_t pos1, size_t pos2)
        : _Base(pos1, pos2)
    {
    }
    inline size_t length() { return std::abs(static_cast< int >(first - second)) + 1; }
};

struct SignalChange {
    unsigned int change;
    unsigned short* target;
};

struct Assertion {
    bool activated;
    bool failed;
    SignalChange trigger;
    SignalChange event;
    Range time;
    std::list<ActivatedPoint> APList;
};
}
