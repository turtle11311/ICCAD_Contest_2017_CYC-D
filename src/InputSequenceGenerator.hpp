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

struct AssertionStatus {
    size_t slack;
    Assertion* target;
    bool suc;
};

typedef std::list< Pattern > InputSequence;

class InputSequenceGenerator : protected FiniteStateMachine {
    typedef FiniteStateMachine _Base;
    friend int(::yyparse)(InputSequenceGenerator& FSM);

public:
    InputSequenceGenerator();
    void simulator();
    void preprocess();
    void staticFindActivatedPoint(Assertion& asrt);
    void printInputSequence();
    void outputAnswer();

private:
    void initial2ActivatedArc();
    void evalInitial2State();
    void assertionInspector(InputSequence& seq);
    void assertionInspectorVersion2();
    void staticFindOutputSignalActivatedPoint(bool trigger, unsigned int index, std::list< ActivatedPoint >& APList);
    void staticFindInputSignalActivatedPoint(bool trigger, unsigned int index, std::list< ActivatedPoint >& APList);
    void fromActivatedPoint2AssertionFailed(Assertion& asrt);
    bool fromActivatedPoint2AssertionOutputSignalFailed(Assertion& asrt, InputSequence& sequence, State* current, Transition* t1, Transition* t2, size_t step);
    void purgeState(int state);
    InputSequence answer;
    InputSequence finalAnswer;
    std::map< Assertion*, InputSequence > answerDict;
    std::list< Assertion > asrtList;
    std::list< ActivatedPoint > path;
    bool asrtFailedFlag = false;
    ActivatedPoint targetAP;
    InputSequence firstHalfAnswer;
    std::list< AssertionStatus > triggeredAssertion;
    std::list< int > rstTable;
};
} // namespace SVParser
