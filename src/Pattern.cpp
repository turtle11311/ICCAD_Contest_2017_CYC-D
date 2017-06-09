#include "Pattern.hpp"

namespace SVParser {
Pattern::Pattern(const size_t size)
    : _Base(size, 0)
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
            this->operator[](pos) = 2;
            break;
        }
    }
}

Pattern::Pattern(const std::string& str)
{
    Pattern new_pattern(str.c_str());
    std::swap(new_pattern, *this);
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
        if ((*this)[i] == 2)
            continue;
        else if ((*this)[i] != rhs[i])
            return false;
    }
    return true;
}

std::ostream& operator<<(std::ostream& os, const Pattern& pattern)
{
    std::string str(pattern.size(), 'x');
    for (int i = 0; i < pattern.size(); ++i) {
        str[i] = pattern[i] < 2 ? pattern[i] + '0' : 'x';
    }
    std::reverse(str.begin(), str.end());
    return os << str;
}
}