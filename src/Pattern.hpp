#pragma once
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <utility>
#include <vector>

namespace SVParser {

class Pattern : private std::vector< unsigned short > {
    typedef std::vector< unsigned short > _Base;

public:
    static const unsigned short DONT_CARE = 2;
    friend std::ostream& operator<<(std::ostream& os, const Pattern& pattern);
    using _Base::size;
    using _Base::operator[];
    using _Base::operator=;
    Pattern(const size_t size = 0);
    Pattern(const Pattern& rhs);
    Pattern(const char* str);
    Pattern(const std::string& str);
    const Pattern& operator=(const Pattern& rhs);
    bool operator==(const Pattern& rhs);
};
}
