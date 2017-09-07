#pragma once
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
    using _Base::value_type;
    using _Base::size;
    using _Base::operator[];
    using _Base::operator=;
    using _Base::push_back;
    Pattern(const size_t size = 0, value_type val = 0);
    Pattern(const Pattern& rhs);
    Pattern(const char* str);
    Pattern(const std::string& str);
    Pattern flipBy(const Pattern& base);
    Pattern intersectionWith(const Pattern& rhs);
    Pattern diffWith(const Pattern& rhs);
    bool empty() const;
    const Pattern& operator=(const Pattern& rhs);
    static Pattern random(size_t size);
    bool operator==(const Pattern& rhs);
    Pattern defaultPattern();
};

class InputPattern : public Pattern {
    typedef Pattern _Base;

public:
    friend std::ostream& operator<<(std::ostream& os, const InputPattern& pattern);
    InputPattern(const size_t size = 0, value_type val = 0, bool reset = false);
    InputPattern(const Pattern& rhs, bool reset = false);
    InputPattern& reset();
    static InputPattern random(size_t size);

    bool _reset;
};
} // namespace SVParser
