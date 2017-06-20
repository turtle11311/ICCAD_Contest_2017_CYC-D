#pragma once
#include "FiniteStateMachine.hpp"
#include <list>
#include <map>

namespace SVParser {

typedef std::list< Pattern > InputSequence;

class InputSequenceGenerator : protected FiniteStateMachine {
    typedef FiniteStateMachine _Base;

public:
    InputSequenceGenerator();
    void preprocess();

private:
    void evalInitial2State();
    void purgeState(int state);
    InputSequence answer;
    std::map< int, InputSequence > initial2ActivetedList;
};
}
