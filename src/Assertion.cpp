#include "Assertion.hpp"
#include <iostream>

using std::cout;
using std::endl;

namespace SVParser {

bool ActivatedPoint::cmpWithLayer(const ActivatedPoint& lhs, const ActivatedPoint& rhs)
{
    return lhs.state->layer < rhs.state->layer;
}

ActivatedPoint::ActivatedPoint()
{
}

ActivatedPoint::ActivatedPoint(State* state, Pattern& pattern1, Pattern& pattern2, Transition* transition1, Transition* transition2)
    : state(state)
    , pattern1(pattern1)
    , pattern2(pattern2)
    , transition1(transition1)
    , transition2(transition2)
{
}

void Assertion::sortActivatedPointByLayer()
{
    APList.sort(&ActivatedPoint::cmpWithLayer);
}

} // namespace SVParser
