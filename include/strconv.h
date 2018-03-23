#ifndef _INCLUDE_UTILS_STRCONV_H_
#define _INCLUDE_UTILS_STRCONV_H_

#include <stdint.h>

namespace cap
{

class StrConv
{
public:
    static inline uint64_t ToUint64(const char* start, const char* end)
    {
        if (NULL == start || NULL == end)
        {
            return 0;
        }

        std::string value(start, end - start);
        std::stringstream ss;
        ss << value;

        uint64_t result = 0;
        ss >> result;
        return result;
    }
};

} // namespace cap

#endif // _INCLUDE_UTILS_STRCONV_H_