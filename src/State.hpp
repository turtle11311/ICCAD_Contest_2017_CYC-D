#pragma once
#include "Pattern.hpp"
#include <climits>
#include <utility>
#include <vector>

namespace SVParser {

class Transition;

class State {
public:
    friend class FiniteStateMachine;
    struct From {
        State* state;
        Transition* transition;
        From(State* state, Transition* transition);
    };
    State(int label);
    ~State();
    bool deadEnd();

    const int label;
    std::vector< Transition* > transitions;
    std::vector< From > fromList;
    int layer;
    bool traversed;
};

class Transition {
    friend std::ostream& operator<<(std::ostream& os, const Transition& transition);

public:
    Transition();
    Transition(const Transition& rhs);
    Transition(Pattern&& pattern, State* nState, Pattern&& out);
    const Transition& operator=(const Transition& rhs);
    InputPattern defaultPattern();

    Pattern pattern;
    Pattern out;
    State* nState;
};
} // namespace SVParser
