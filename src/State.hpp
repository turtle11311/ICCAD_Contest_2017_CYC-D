#pragma once
#include "Pattern.hpp"
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
    inline int label() { return _label; };

private:
    const int _label;
    std::vector< Transition* > transitions;
    std::vector< From > fromList;
};

class Transition {
    friend std::ostream& operator<<(std::ostream& os, const Transition& transition);

public:
    Transition();
    Transition(const Transition& rhs);
    Transition(Pattern&& pattern, State* nState, Pattern&& out);
    const Transition& operator=(const Transition& rhs);
    Pattern defaultPattern();

    Pattern pattern;
    Pattern out;
    State* nState;
};
}