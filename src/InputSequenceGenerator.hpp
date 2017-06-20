#pragma once
#include "Assertion.hpp"
#include "FiniteStateMachine.hpp"
#include <list>
#include <map>

namespace SVParser {
class InputSequenceGenerator;
}

extern int yyparse(SVParser::InputSequenceGenerator& FSM);

namespace SVParser {

typedef std::list< Pattern > InputSequence;

class InputSequenceGenerator : protected FiniteStateMachine {
    typedef FiniteStateMachine _Base;
    friend int(::yyparse)(InputSequenceGenerator& FSM);

public:
    InputSequenceGenerator();
    void preprocess();
    void staticFindActivatedPoint(Assertion& asrt);

private:
    void evalInitial2State();
    void staticFindOutputSignalActivatedPoint(bool trigger, unsigned int index, std::list< ActivatedPoint >& APList);
    void staticFindInputSignalActivatedPoint(bool trigger, unsigned int index, std::list< ActivatedPoint >& APList);
    void purgeState(int state);
    InputSequence answer;
    std::map< int, InputSequence > initial2ActivetedList;
    std::list< Assertion > asrtList;
};
}
