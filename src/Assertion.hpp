#pragma once
#include "Range.hpp"
#include "State.hpp"
#include <cstdlib>
#include <iostream>
#include <list>
using std::cout;
using std::endl;

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
    void printAP()
    {
        cout << "(S" << state->label << ") -> " << pattern1
             << " | out: " << transition1->out << " => (S" << transition1->nState->label
             << ") -> " << pattern2 << " | out: " << transition2->out << endl;
    }
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
