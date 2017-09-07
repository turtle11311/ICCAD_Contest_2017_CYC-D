#pragma once
#include "Assertion.hpp"
#include "FiniteStateMachine.hpp"
#include <initializer_list>
#include <fstream>
#include <list>
#include <map>
#include <set>

namespace SVParser {
class InputSequenceGenerator;
}

extern int yyparse(SVParser::InputSequenceGenerator& FSM);

namespace SVParser {

struct AssertionStatus {
    size_t slack;
    Assertion* target;
    bool suc;
    Transition* trans1, *trans2;
};

typedef std::vector< InputPattern > InputSequence;

class InputSequenceGenerator : protected FiniteStateMachine {
    typedef FiniteStateMachine _Base;
    friend int(::yyparse)(InputSequenceGenerator& FSM);

public:
    InputSequenceGenerator();
    ~InputSequenceGenerator();
    void simulator();
    void preprocess();
    void staticFindActivatedPoint(Assertion& asrt);
    void outputAnswer();
    void evalInputSequence(std::string filename);

private:
    void initial2ActivatedArc();
    void evalInitial2State();
    void assertionInspector(InputSequence& seq);
    void staticFindOutputSignalActivatedPoint(bool trigger, unsigned int index, std::list< ActivatedPoint >& APList);
    void staticFindInputSignalActivatedPoint(bool trigger, unsigned int index, std::list< ActivatedPoint >& APList);
    void fromActivatedPoint2AssertionFailed(Assertion& asrt);
    bool fromActivatedPoint2AssertionInputSignalFailed(Assertion& asrt, InputSequence& sequence);
    bool fromActivatedPoint2AssertionOutputSignalFailed(Assertion& asrt, InputSequence& sequence, State* current,
                                                        Transition* t1, Transition* t2,
                                                        size_t step);
    bool fromActivatedPoint2AssertionOutputSignalFailed(Assertion& asrt, InputSequence& sequence, State* current, State* nState,
                                                        Transition* t1, Transition* t2,
                                                        size_t step, size_t bound);
    void purgeState(int state);
    void simulatedAnnealing();
    void randomSwap4SA(int, int);
    void generateSolution();
    void assertionByOrder(std::initializer_list< int > order);
    InputPattern evalSecondInput();
    InputPattern evalStartInput();
    InputSequence answer, finalAnswer;
    std::map< Assertion*, InputSequence > answerDict;
    std::vector< Assertion* > asrtList;
    std::vector< Assertion* >::iterator upcomingAsrt;
    std::list< ActivatedPoint > path;
    bool asrtFailedFlag = false;
    ActivatedPoint targetAP;
    InputSequence firstHalfAnswer;
    std::list< AssertionStatus > triggeredAssertion;
    std::list< int > rstTable;
    std::ofstream coverage, act;
    Assertion* careAsrt;
    std::set< Assertion* > failedAssertion;
};
} // namespace SVParser
