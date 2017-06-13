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
        cout << "(S" << it->state->label << ") -> " << it->pattern1
             << " | out: " << it->transition1->out << " => (S" << it->transition1->nState->label
             << ") -> " << it->pattern2 << " | out: " << it->transition2->out << endl;
    }
    cout << endl
         << endl;
}
}
