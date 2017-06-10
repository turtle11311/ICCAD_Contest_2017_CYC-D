#include "State.hpp"
#include <algorithm>
#include <memory>

namespace SVParser {
State::From::From(State* state, Transition* transition)
    : state(state)
    , transition(transition)
{
}

State::State(int label)
    : _label(label)
{
}

State::~State()
{
    std::for_each(transitions.begin(), transitions.end(), std::default_delete<Transition>());
}

Transition::Transition() {}

Transition::Transition(const Transition& rhs)
    : pattern(rhs.pattern)
    , nState(rhs.nState)
    , out(rhs.out)
{
}

Transition::Transition(Pattern&& pattern, State* nState, Pattern&& out)
    : pattern(pattern)
    , nState(nState)
    , out(out)
{
}

const Transition& Transition::operator=(const Transition& rhs)
{
    Transition cpy(rhs);
    std::swap(pattern, cpy.pattern);
    std::swap(nState, cpy.nState);
    std::swap(out, cpy.out);
    return *this;
}

Pattern Transition::defaultPattern()
{
    Pattern res = pattern;
    for (int i = 0; i < res.size(); ++i)
        if (res[i] == 2)
            res[i] = 0;
    return res;
}

std::ostream& operator<<(std::ostream& os, const Transition& transition)
{
    return os << "Match: " << transition.pattern << " then go to State=>" << transition.nState->label() << " output: " << transition.out;
}
}