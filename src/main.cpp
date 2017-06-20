#include "InputSequenceGenerator.hpp"
#include "SVParser.hpp"
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <vector>

using namespace SVParser;
using std::cout;
using std::endl;

extern FILE* yyin;
extern std::list< Assertion > asrtList;

std::ofstream output;

int ptnSize;
int* state = new int;
std::vector< Pattern > inputSequence;
std::vector< unsigned int > rstRecord;
std::vector< int > layerTable;
std::map< int, std::list< int > > rlayerTable;
std::list< ActivatedPoint > path;
bool asrtFailedFlag = false;

void parseArgAndInitial(int argc, char* argv[]);

int main(int argc, char* argv[])
{
    parseArgAndInitial(argc, argv);

    InputSequenceGenerator generator;

    // speed up c++ STL I/O
    std::ios_base::sync_with_stdio(false);

    generator.evalInitial2State();

    delete state;
    return EXIT_SUCCESS;
}

void parseArgAndInitial(int argc, char* argv[])
{
    if (argc < 4) {
        fprintf(stderr, "Usage: %s -i input_file -o output_file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char opt;
    while ((opt = getopt(argc, argv, "i:o:")) != EOF) {
        switch (opt) {
        case 'i':
            yyin = fopen(optarg, "r");
            break;
        case 'o':
            output.open(optarg, std::ios::out);
            break;
        default:
            break;
        }
    }
}
