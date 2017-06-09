#pragma once
#include "Pattern.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <utility>
#include <vector>

namespace SVParser {

class Range : public std::pair< size_t, size_t > {
    typedef std::pair< size_t, size_t > _Base;

public:
    Range(size_t pos = 0)
        : _Base(pos, pos)
    {
    }
    Range(size_t pos1, size_t pos2)
        : _Base(pos1, pos2)
    {
    }
    inline size_t length() { return std::abs(static_cast< int >(first - second)) + 1; }
};

struct Transition {
    Transition(){};
    Transition(const Transition& rhs)
        : pattern(rhs.pattern)
        , nstate(rhs.nstate)
        , out(rhs.out)
    {
    }
    Transition(const char* pattern, int nstate, const char* out)
        : pattern(pattern)
        , nstate(nstate)
        , out(out)
    {
    }
    const Transition& operator=(const Transition& rhs)
    {
        Transition cpy(rhs);
        std::swap(pattern, cpy.pattern);
        std::swap(nstate, cpy.nstate);
        std::swap(out, cpy.out);
        return *this;
    }
    Pattern pattern;
    int nstate;
    Pattern out;
};

struct SignalChange {
    unsigned int change;
    unsigned short* target;
};

struct Assertion {
    SignalChange trigger;
    SignalChange event;
    Range time;
};
}