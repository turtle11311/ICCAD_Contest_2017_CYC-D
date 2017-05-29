#include <vector>
#include <utility>
#include <cstdlib>

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
    using std::vector<Ty>::size;
    Pattern()
        : std::vector<Ty>(0, 0) {}
    Pattern(size_t size)
        : std::vector<Ty>(size, 0) {}
};

}