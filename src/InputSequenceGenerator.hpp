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
    void simulator();
    void preprocess();
    void staticFindActivatedPoint(Assertion& asrt);
    void printPath();

private:
    void evalInitial2State();
    void staticFindOutputSignalActivatedPoint(bool trigger, unsigned int index, std::list< ActivatedPoint >& APList);
    void staticFindInputSignalActivatedPoint(bool trigger, unsigned int index, std::list< ActivatedPoint >& APList);
    void fromActivatedPoint2AssertionFailed(Assertion& asrt);
    void findOutputSignalTermiateStartPoint(bool triggerFlag, unsigned int index, ActivatedPoint& ap, Range& range);
    void recursiveTraverseOS(std::list< ActivatedPoint > stack, bool triggerFlag, unsigned int index, unsigned int cycle);
    void findInputSignalTermiateStartPoint(bool triggerFlag, unsigned int index, ActivatedPoint& ap, Range& range);
    void usingDijkstraForNonWeightedGraph(ActivatedPoint& ap);
    std::list< ActivatedPoint > integerPath2APPathConverter(std::list< int >& shortestPath, ActivatedPoint& ap);
    void purgeState(int state);
    InputSequence answer;
    std::map< int, InputSequence > initial2ActivetedList;
    std::list< Assertion > asrtList;
    std::list< ActivatedPoint > path;
    bool asrtFailedFlag = false;
};
}
