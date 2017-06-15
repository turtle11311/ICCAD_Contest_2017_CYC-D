#include "Assertion.hpp"
#include <iostream>

using std::cout;
using std::endl;

namespace SVParser {

bool ActivatedPoint::cmpWithLayer(const ActivatedPoint& lhs, const ActivatedPoint& rhs)
{
    return lhs.state->layer < rhs.state->layer;
}

void Assertion::sortActivatedPointByLayer()
{
    APList.sort(&ActivatedPoint::cmpWithLayer);
}

void Assertion::printActivatedPoint()
{
    cout << APList.size() << endl;
    for (auto it = APList.begin(); it != APList.end(); ++it) {
        it->printAP();
    }
    cout << endl
         << endl;
}
}
