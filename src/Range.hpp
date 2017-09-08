#pragma once
#include <cstdlib>
#include <utility>

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
    inline size_t length() const
    {
        return std::abs(static_cast< int >(first - second)) + 1;
    }
};
} // namespace SVParser
