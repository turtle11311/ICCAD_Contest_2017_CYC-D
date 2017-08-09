#include "State.hpp"
#include <algorithm>
#include <climits>
#include <memory>

namespace SVParser {
State::From::From(State* state, Transition* transition)
    : state(state)
    , transition(transition)
{
}

State::State(int label)
    : label(label)
    , layer(INT_MAX)
    , traversed(false)
{
}

State::~State()
{
    std::for_each(transitions.begin(), transitions.end(), std::default_delete< Transition >());
}

Transition::Transition()
{
}

Transition::Transition(const Transition& rhs)
    : pattern(rhs.pattern)
    , nState(rhs.nState)
    , out(rhs.out)
{
}

Transition::Transition(InputPattern&& pattern, State* nState, Pattern&& out)
    : pattern(pattern)
    , nState(nState)
    , out(out)
{
}

Transition::Transition(InputPattern& pattern, State* nState, Pattern& out)
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

InputPattern Transition::defaultPattern()
{
    return pattern.defaultPattern();
}

std::ostream& operator<<(std::ostream& os, const Transition& transition)
{
    const Pattern& match = transition.pattern,
                   &out = transition.out;
    return os << match << ": begin nstate=S" << transition.nState->label << "; out=" << out << "; end";
}
} // namespace SVParser
