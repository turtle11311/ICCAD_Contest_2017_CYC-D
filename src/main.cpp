#include "InputSequenceGenerator.hpp"
#include <boost/filesystem.hpp>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <vector>

namespace fs = boost::filesystem;

using namespace SVParser;
using std::cout;
using std::endl;

extern FILE* yyin;

std::ofstream output;
fs::path caseDir;
fs::path debuginfoDir;

int assertionID = 0;

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

    generator.preprocess();
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
    while ((opt = getopt(argc, argv, "i:o:t:")) != EOF) {
        switch (opt) {
        case 'i':
            yyin = fopen(optarg, "r");
            break;
        case 'o':
            output.open(optarg, std::ios::out);
            caseDir = fs::absolute(optarg).parent_path();
#ifdef DEBUG
            cout << "Creating debug directory is " << std::boolalpha << fs::create_directory(caseDir.append("asrt")) << endl;
#endif
            break;
        case 't':
            assertionID = atoi(optarg);
            break;
        default:
            break;
        }
    }
}
