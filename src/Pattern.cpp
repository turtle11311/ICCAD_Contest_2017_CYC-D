#include "Pattern.hpp"
#include <algorithm>

namespace SVParser {

Pattern::Pattern(const size_t size, value_type val)
    : _Base(size, val)
{
}
Pattern::Pattern(const Pattern& rhs)
    : _Base(rhs)
{
}
Pattern::Pattern(const char* str)
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
            this->operator[](pos) = DONT_CARE;
            break;
        }
    }
}

Pattern::Pattern(const std::string& str)
{
    Pattern new_pattern(str.c_str());
    std::swap(new_pattern, *this);
}

Pattern Pattern::random(size_t size)
{
    Pattern res(size);
    for (size_t i = 0; i < res.size(); ++i)
        res[i] = rand() % 2;
    return res;
}

const Pattern& Pattern::operator=(const Pattern& rhs)
{
    _Base::operator=(rhs);
    return *this;
}

bool Pattern::operator==(const Pattern& rhs)
{
    if (size() != rhs.size())
        return false;
    for (size_t i = 0; i < size(); ++i) {
        if ((*this)[i] == DONT_CARE)
            continue;
        else if ((*this)[i] != rhs[i])
            return false;
    }
    return true;
}

Pattern Pattern::defaultPattern()
{
    Pattern res = *this;
    for (int i = 0; i < res.size(); ++i)
        if (res[i] == 2)
            res[i] = 0;
    return res;
}

std::ostream& operator<<(std::ostream& os, const Pattern& pattern)
{
    std::string str(pattern.size(), 'x');
    for (int i = 0; i < pattern.size(); ++i) {
        str[i] = pattern[i] != Pattern::DONT_CARE ? pattern[i] + '0' : 'x';
    }
    std::reverse(str.begin(), str.end());
    return os << str;
}

InputPattern::InputPattern(const size_t size, value_type val, bool reset)
    : _Base(size, val)
    , _reset(reset)
{
}

InputPattern::InputPattern(const Pattern& rhs, bool reset)
    : _Base(rhs)
    , _reset(reset)
{
}

InputPattern InputPattern::random(size_t size)
{
    return InputPattern(_Base::random(size));
}

InputPattern& InputPattern::reset()
{
    this->_reset = true;
    return *this;
}

std::ostream& operator<<(std::ostream& os, const InputPattern& pattern)
{
    return os << pattern._reset << static_cast< Pattern >(pattern);
}
} // namespace SVParser
