#pragma once
#include "FiniteStateMachine.hpp"
#include "Pattern.hpp"
#include "State.hpp"
#include <cstdlib>
#include <list>
#include <utility>
using std::endl;
using std::cout;

namespace SVParser {

struct ActivatedPoint {
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
    std::list< ActivatedPoint > APList;
    void sortActivatedPointByLayer()
    {
        APList.sort([](const ActivatedPoint& lhs, const ActivatedPoint& rhs) {
            return lhs.state->layer < rhs.state->layer;
        });
    }
    void printActivatedPoint()
    {
        cout << APList.size() << endl;
        for (auto it = APList.begin(); it != APList.end(); ++it) {
            cout << "(S" << it->state->label << ") -> " << it->pattern1
                 << " | out: " << it->transition1->out << " => (S" << it->transition1->nState->label
                 << ") -> " << it->pattern2 << " | out: " << it->transition2->out << endl;
        }
        cout << endl
             << endl;
    }
};
}
