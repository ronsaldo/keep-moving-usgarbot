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

    uint8_t size_;
    char data[DataSize];

public:
    typedef SmallFixedString<N> SelfType;

    SmallFixedString() = default;

    SmallFixedString(const std::string &s)
    {
        size_ = std::min(size_t(DataSize), s.size());
        memcpy(data, s.data(), size_);
    }

    operator std::string() const
    {
        return std::string(data, data + size_);
    }

    size_t size() const
    {
        return size_;
    }

    bool operator ==(const SelfType &o) const
    {
        return size_ == o.size_ && memcmp(data, o.data, size_) == 0;
    }

    bool operator !=(const SelfType &o) const
    {
        return !(*this == o);
    }
};

#endif //FIXED_STRING_HPP
