#ifndef FIXED_VECTOR_HPP
#define FIXED_VECTOR_HPP

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <utility>

template<typename VT, int MC>
class FixedVector
{
    size_t size_;
    alignas(VT) uint8_t storageBuffer[MC*sizeof(VT)];

    VT *storage()
    {
        return reinterpret_cast<VT*> (storageBuffer);
    }

    const VT *storage() const
    {
        return reinterpret_cast<const VT*> (storageBuffer);
    }

public:
    enum {
        MaxCapacity = MC,
    };

    typedef VT value_type;
    typedef value_type * iterator;
    typedef value_type const * const_iterator;

    FixedVector()
        : size_(0) {}

    void clear()
    {
        for(size_t i = 0; i < size_; ++i)
            storage()[i].~VT();
        size_ = 0;
    }

    size_t capacity() const
    {
        return MaxCapacity;
    }

    size_t size() const
    {
        return size_;
    }

    bool empty() const
    {
        return size_ == 0;
    }

    void push_back(const value_type &v)
    {
        if(size_ >= MaxCapacity)
            return;

        new (&storage()[size_]) VT (v);
        ++size_;
    }

    void pop_back()
    {
        if(empty())
            return;

        storage()[size_-1].~VT();
        --size_;
    }

    template<typename FT>
    void removeAllThat(const FT &f)
    {
        size_t destIndex = 0;
        for(size_t i = 0; i < size_; ++i)
        {
            auto &element = storage()[i];
            if(!f(element))
            {
                if(i != destIndex)
                    storage()[destIndex] = std::move(element);
                ++destIndex;
            }
        }

        size_ = destIndex;
    }

    value_type &back()
    {
        return storage()[size_ - 1];
    }

    const value_type &back() const
    {
        return storage()[size_ - 1];
    }

    iterator begin()
    {
        return storage();
    }

    iterator end()
    {
        return storage() + size_;
    }

    const_iterator begin() const
    {
        return storage();
    }

    const_iterator end() const
    {
        return storage() + size_;
    }

    bool includes(const value_type &element) const
    {
        for(const auto &each : *this)
        {
            if(each == element)
                return true;
        }

        return false;
    }

};

#endif //FIXED_VECTOR_HPP
