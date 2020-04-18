#ifndef FIXED_STRING_HPP
#define FIXED_STRING_HPP

#include <string>
#include <string.h>

template<int N>
class SmallFixedString
{
private:
    enum {
        DataSize = N - 1
    };

    uint8_t size;
    char data[DataSize];

public:
    typedef SmallFixedString<N> SelfType;

    SmallFixedString() = default;

    SmallFixedString(const std::string &s)
    {
        size = std::min(size_t(DataSize), s.size());
        memcpy(data, s.data(), size);
    }

    operator std::string() const
    {
        return std::string(data, data + size);
    }

    bool operator ==(const SelfType &o) const
    {
        return size == o.size && memcmp(data, o.data, size) == 0;
    }

    bool operator !=(const SelfType &o) const
    {
        return !(*this == o);
    }
};

#endif //FIXED_STRING_HPP
