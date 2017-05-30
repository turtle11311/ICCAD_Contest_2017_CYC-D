#include <algorithm>
#include <iostream>
#include <vector>
#include <utility>
#include <cstdlib>
#include <cstring>

namespace SVParser
{

struct range : public std::pair<size_t, size_t>
{
  public:
    range(size_t pos)
        : pair(pos, pos) {}
    range(size_t pos1, size_t pos2)
        : pair(pos1, pos2) {}
    inline size_t length() { return first - second + 1; }
};

template <typename Ty = unsigned short>
class Pattern : private std::vector<Ty>
{
  public:
    template <typename U>
    friend std::ostream& operator<<(std::ostream& os, const Pattern<U> &pattern);
    using std::vector<Ty>::size;
    Pattern(const size_t size = 0)
        : std::vector<Ty>(size, 0) {}
    Pattern(const Pattern& rhs)
        : std::vector<Ty>(rhs) {}
    Pattern(const char* str)
        : std::vector<Ty>(strlen(str))
    {
        for (int i = 0; i < size(); ++i) {
            int pos = size() - i - 1;
            switch(str[i]) {
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
        : Pattern<Ty>(str.c_str()) {}

    bool operator==(const Pattern& rhs)
    {
        if (size() != rhs.size()) return false;
        for (size_t i = 0; i < size(); ++i) {
            if ((*this)[i] == 2) continue;
            else if ((*this)[i] != rhs[i]) return false;
        }
        return true;
    }
};

template <typename U>
std::ostream& operator<<(std::ostream& os, const Pattern<U> &pattern)
{
    std::string str(pattern.size(), 'x');
    for (int i = 0; i < pattern.size(); ++i) {
        str[i] = pattern[i] < 2 ? pattern[i] + '0' : 'x';
    }
    std::reverse(str.begin(), str.end());
    return os << str;
}

}