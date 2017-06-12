#pragma once
#include "FiniteStateMachine.hpp"
#include "Pattern.hpp"
#include <cstdlib>
#include <utility>

namespace SVParser {

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

struct Assertion {
    enum class TargetType {
        IN,
        OUT
    };
    enum class SignalEdge {
        ROSE,
        FELL,
        STABLE
    };
    struct SignalChange {
        Assertion::SignalEdge change;
        Assertion::TargetType target;
        size_t index;
    };

    bool activated;
    bool failed;
    SignalChange trigger;
    SignalChange event;
    Range time;
};
}
