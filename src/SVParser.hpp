#pragma once
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

template < typename Ty = unsigned short >
class Pattern : private std::vector< Ty > {
    typedef std::vector< Ty > _Base;

public:
    template < typename U >
    friend std::ostream& operator<<(std::ostream& os, const Pattern< U >& pattern);
    using _Base::size;
    using _Base::operator[];
    using _Base::operator=;
    Pattern(const size_t size = 0)
        : _Base(size, 0)
    {
    }
    Pattern(const Pattern& rhs)
        : _Base(rhs)
    {
    }
    Pattern(const char* str)
        : _Base(strlen(str))
    {
        for (int i = 0; i < size(); ++i) {
            int pos = size() - i - 1;
            switch (str[i]) {
            case '0':
            case '1':
                this->operator[](pos) = str[i] == '0' ? 0 : 1;
                break;
            default:
                this->operator[](pos) = 2;
                break;
            }
        }
    }

    Pattern(const std::string& str)
    {
        Pattern new_pattern(str.c_str());
        std::swap(new_pattern, *this);
    }

    const Pattern& operator=(const Pattern& rhs)
    {
        _Base::operator=(rhs);
        return *this;
    }

    bool operator==(const Pattern& rhs)
    {
        if (size() != rhs.size())
            return false;
        for (size_t i = 0; i < size(); ++i) {
            if ((*this)[i] == 2)
                continue;
            else if ((*this)[i] != rhs[i])
                return false;
        }
        return true;
    }
};

template < typename U >
std::ostream& operator<<(std::ostream& os, const Pattern< U >& pattern)
{
    std::string str(pattern.size(), 'x');
    for (int i = 0; i < pattern.size(); ++i) {
        str[i] = pattern[i] < 2 ? pattern[i] + '0' : 'x';
    }
    std::reverse(str.begin(), str.end());
    return os << str;
}

struct Transition {
    Transition(){};
    Transition(const Transition& rhs)
        : pattern(rhs.pattern)
        , nstate(rhs.nstate)
        , out(rhs.out)
        , traversed(false)
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
    Pattern<> defaultPattern()
    {
        Pattern<> res = pattern;
        for ( int i = 0 ; i < res.size() ; ++i )
            if ( res[i] == 2 ) res[i] = 0;
        return res;
    }

    Pattern<> pattern;
    int nstate;
    Pattern<> out;
    bool traversed;
};

struct SignalChange {
    unsigned int change;
    unsigned short* target;
};

struct Assertion {
    bool activated;
    bool failed;
    SignalChange trigger;
    SignalChange event;
    Range time;
};
}
