#pragma once
#include "Range.hpp"
#include "State.hpp"
#include <cstdlib>
#include <list>

namespace SVParser {

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
    SignalEdge change;
    TargetType target;
    size_t index;
};

struct ActivatedPoint {
    State* state;
    Pattern pattern1, pattern2;
    Transition *transition1, *transition2;
};

struct Assertion {

    bool activated;
    bool failed;
    SignalChange trigger;
    SignalChange event;
    Range time;
    std::list< ActivatedPoint > APList;
    void sortActivatedPointByLayer();
    void printActivatedPoint();
};
}
