#pragma once
#include "Range.hpp"
#include "State.hpp"
#include <cstdlib>
#include <iostream>
#include <list>
#include <string>
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
    ActivatedPoint();
    ActivatedPoint(State* state, Pattern& pattern1, Pattern& pattern2, Transition* transition1, Transition* transition2);
    State* state;
    Pattern pattern1, pattern2;
    Transition *transition1, *transition2;
    void printAP()
    {
        cout << "(S" << state->label << ") -> " << pattern1
             << " | out: " << transition1->out << " => (S" << transition1->nState->label
             << ") -> " << pattern2 << " | out: " << transition2->out << endl;
    }
    static bool cmpWithLayer(const ActivatedPoint& lhs, const ActivatedPoint& rhs);
};

struct Assertion {

    std::string name;
    bool activated;
    bool failed;
    bool noSolution;
    SignalChange trigger;
    SignalChange event;
    Range time;
    std::list< ActivatedPoint > APList;
    void sortActivatedPointByLayer();
    void printActivatedPoint();
};
} // namespace SVParser
