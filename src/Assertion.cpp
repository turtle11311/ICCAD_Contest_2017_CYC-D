#include "Assertion.hpp"
#include <iostream>

using std::cout;
using std::endl;

namespace SVParser {

void Assertion::sortActivatedPointByLayer()
{
    APList.sort([](const ActivatedPoint& lhs, const ActivatedPoint& rhs) {
        return lhs.state->layer < rhs.state->layer;
    });
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
