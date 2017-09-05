#include "InputSequenceGenerator.hpp"
#include <log4cxx/logger.h>
#include <log4cxx/propertyconfigurator.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

std::ofstream output;

int ptnSize;
int* state = new int;

bool openSA = true;
bool readfile = false;
std::string filename;

std::vector< Pattern > inputSequence;
std::vector< unsigned int > rstRecord;
std::vector< int > layerTable;
std::map< int, std::list< int > > rlayerTable;
std::list< ActivatedPoint > path;
bool asrtFailedFlag = false;

void parseArgAndInitial(int argc, char* argv[]);

int main(int argc, char* argv[])
{
    log4cxx::PropertyConfigurator::configure("log4cxx.properties");
    srand(time(0));
    parseArgAndInitial(argc, argv);

    InputSequenceGenerator generator;

    // speed up c++ STL I/O
    std::ios_base::sync_with_stdio(false);

    generator.preprocess();

    if (readfile) {
        generator.evalInputSequence(filename);
        return EXIT_SUCCESS;
    }

    generator.simulator();

    generator.outputAnswer();

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
    while ((opt = getopt(argc, argv, "i:o:s:q")) != EOF) {
        switch (opt) {
        case 'i':
            yyin = fopen(optarg, "r");
            break;
        case 'o':
            output.open(optarg, std::ios::out);
            break;
        case 'q':
            openSA = false;
            break;
        case 's':
            readfile = true;
            filename = std::string(optarg);
            break;
        default:
            break;
        }
    }
}
