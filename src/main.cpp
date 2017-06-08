#include "SVParser.hpp"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <vector>

using namespace SVParser;
using std::cout;
using std::endl;

extern int yyparse(void);

extern std::map< std::string, Pattern<>* > varMap;
extern std::map< std::string, unsigned int > parameterTable;
extern std::map< int, std::list< Transition > > fsm;
extern std::list< Assertion > asrtList;

struct ActivatedPoint1 {
    int state1, state2;
    Transition *transition1, *transition2;
};
struct ActivatedPoint2 {
    int state1, state2;
    Pattern<> p1, p2;
    Transition *transition1, *transition2;
};

int ptnSize;
int* state = new int;
std::vector< Pattern<> > inputSequence;
std::vector< unsigned int > rstRecord;
std::list< ActivatedPoint1 > OSAPList;
std::list< ActivatedPoint2 > ISAPList;

void initializer();
void simulator();
void staticFindActivatedPoint(Assertion&);
void staticFindOutputSignalActivatedPoint(bool, unsigned int);
void staticFindInputSignalActivatedPoint(bool, unsigned int);
std::pair< bool, unsigned int > find(unsigned short*);
void printInputSequence();
void printOutputSequence();
void printActivatedPoint(int);
int main(int argc, const char* argv[])
{
    yyparse();
    ptnSize = fsm[0].front().pattern.size();
    for (int i = 0; i < fsm.size(); ++i) {
        fsmIt[i] = fsm[i].begin();
    }
    simulator();
    for (auto it = varMap.begin(); it != varMap.end(); ++it) {
        delete it->second;
    }
    delete state;
    return EXIT_SUCCESS;
}

void initializer()
{
    inputSequence.clear();
    inputSequence.push_back(Pattern<>(ptnSize));
    (*state) = 0;
}

void simulator()
{
    int count = 0;
    for (auto it = asrtList.begin(); it != asrtList.end(); ++it) {
        cout << "Assertion: " << ++count << endl;
        initializer();
        staticFindActivatedPoint(*it);
    }
}

void staticFindActivatedPoint(Assertion& asrt)
{
    std::pair< bool, unsigned int > io = find(asrt.trigger.target);
    bool triggerFlag = io.first;
    unsigned int index = io.second;
    cout << "Activated target: " << ((triggerFlag) ? "out[" : "in[") << index << "]"
         << " is " << (triggerFlag ? "rose" : "fell") << "." << endl;
    if (triggerFlag)
        staticFindOutputSignalActivatedPoint(triggerFlag, index);
    else
        staticFindInputSignalActivatedPoint(triggerFlag, index);
}

void staticFindOutputSignalActivatedPoint(bool triggerFlag, unsigned int index)
{
    cout << "output-signal-activated assertion." << endl;
    for (int i = 0; i < fsm.size(); ++i) {
        for (auto it1 = fsm[i].begin(); it1 != fsm[i].end(); ++it1) {
            if (it1->out[index] == !triggerFlag) {
                for (auto it2 = fsm[it1->nstate].begin(); it2 != fsm[it1->nstate].end(); ++it2) {
                    if (it2->out[index] == triggerFlag) {
                        OSAPList.push_back(ActivatedPoint1({ i, it1->nstate, &(*it1), &(*it2) }));
                    }
                }
            }
        }
    }
    printActivatedPoint(true);
}

void staticFindInputSignalActivatedPoint(bool triggerFlag, unsigned int index)
{
    cout << "input-signal-activated assertion." << endl;
    for (int i = 0; i < fsm.size(); ++i) {
        for (auto it1 = fsm[i].begin(); it1 != fsm[i].end(); ++it1) {
            Pattern<> expectedPattern1 = it1->pattern;
            if (expectedPattern1[index] == !triggerFlag || expectedPattern1[index] == 2)
                expectedPattern1[index] = !triggerFlag;
            else
                continue;
            if (it1->pattern == expectedPattern1) {
                for (auto it2 = fsm[it1->nstate].begin(); it2 != fsm[it1->nstate].end(); ++it2) {
                    Pattern<> expectedPattern2 = it2->pattern;
                    if (expectedPattern2[index] == triggerFlag || expectedPattern2[index] == 2)
                        expectedPattern2[index] = triggerFlag;
                    else
                        continue;
                    if (it2->pattern == expectedPattern2) {
                        ISAPList.push_back(ActivatedPoint2({ i, it1->nstate, expectedPattern1, expectedPattern2, &(*it1), &(*it2) }));
                    }
                }
            }
        }
    }
    printActivatedPoint(false);
}

std::pair< bool, unsigned int > find(unsigned short* target)
{
    std::pair< bool, unsigned int > res;
    if (target >= &(*varMap["out"])[0]) {
        res.first = true;
        res.second = (target - &(*varMap["out"])[0]) * 2 / sizeof(unsigned short);
    } else {
        res.first = false;
        res.second = (target - &(*varMap["in"])[0]) * 2 / sizeof(unsigned short);
    }
    return res;
}

void printInputSequence()
{
    cout << "Result: \n";
    for (auto it = inputSequence.begin(); it != inputSequence.end(); ++it)
        cout << *it << endl;
}

void printOutputSequence()
{
    cout << "Current output sequence: ";
    for (unsigned int i = 0; i < varMap["out"]->size(); ++i) {
        cout << (*varMap["out"])[i];
    }
    cout << endl;
}

void printActivatedPoint(int mode)
{
    if (mode) {
        for (auto it = OSAPList.begin(); it != OSAPList.end(); ++it) {
            cout << "(S" << it->state1 << ") -> " << it->transition1->pattern
                 << " | out: " << it->transition1->out << " => (S" << it->state2
                 << ") -> " << it->transition2->pattern << " | out: " << it->transition2->out << endl;
            ;
        }
    } else {
        for (auto it = ISAPList.begin(); it != ISAPList.end(); ++it) {
            cout << "(S" << it->state1 << ") -> " << it->p1
                 << " | out: " << it->transition1->out << " => (S" << it->state2
                 << ") -> " << it->p2 << " | out: " << it->transition2->out << endl;
        }
    }
    cout << endl
         << endl;
}
