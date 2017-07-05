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
    void printPath();
    void convertPath2InputSequence();
    void printInputSequence();
    void outputNthAssertion(int n);

private:
    void evalInitial2State();
    void assertionInspector(InputSequence& seq);
    void staticFindOutputSignalActivatedPoint(bool trigger, unsigned int index, std::list< ActivatedPoint >& APList);
    void staticFindInputSignalActivatedPoint(bool trigger, unsigned int index, std::list< ActivatedPoint >& APList);
    void fromActivatedPoint2AssertionFailed(Assertion& asrt);
    bool fromActivatedPoint2AssertionOutputSignalFailed(Assertion& asrt, InputSequence& sequence, State* current, Transition* t1, Transition* t2, size_t step);
    void findOutputSignalTermiateStartPoint(bool triggerFlag, unsigned int index, ActivatedPoint& ap, Range& range);
    void recursiveTraverseOS(std::list< ActivatedPoint > stack, bool triggerFlag, unsigned int index, unsigned int cycle);
    void findInputSignalTermiateStartPoint(bool triggerFlag, unsigned int index, ActivatedPoint& ap, Range& range);
    void usingDijkstraForNonWeightedGraph(ActivatedPoint& ap);
    void purgeState(int state);
    void recursiveDFS();
    InputSequence answer;
    std::map< Assertion*, InputSequence > answerDict;
    std::list< Assertion > asrtList;
    std::list< ActivatedPoint > path;
    bool asrtFailedFlag = false;
    std::list< State* > recPath;
    ActivatedPoint targetAP;
    InputSequence firstHalfAnswer;
    std::list< AssertionStatus > triggeredAssertion;
    bool found;
};
} // namespace SVParser
