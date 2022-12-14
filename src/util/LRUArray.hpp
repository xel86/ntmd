#pragma once

#include <algorithm>
#include <deque>

namespace ntmd {

/* Least Recently Used Array for use as a cache.
 * Requires a set size in the constructor.
 * Duplicate elements will be discarded.
 * The most recently pushed item will be in the front of the array, and if the cache is at max
 * capacity the last element will be discarded.*/
template <class T>
class LRUArray
{
  public:
    LRUArray() = delete;
    LRUArray(std::size_t size) : mSize(size){};
    ~LRUArray() = default;

    /* Updates the front of the LRU array with the new item.
     * Least recently used element (back of array) is discarded. */
    void update(T item)
    {
        /* Allow for a cache size of 0 to essentially just be a disabled container
         * without changing exterior code. */
        if (mSize == 0)
        {
            return;
        }

        if (mDeque.empty())
        {
            mDeque.push_front(item);
            return;
        }

        /* If the item is already in the front of the cache, don't change it. */
        if (mDeque.front() != item)
        {
            /* If the newly found item was already in the cache but wasn't in the front,
             * erase it and put it in the front. */
            const auto inCache = std::find(mDeque.begin(), mDeque.end(), item);
            if (inCache != mDeque.end())
            {
                mDeque.erase(inCache);
            }

            /* Once we have filled our cache, replace the least recently used item. */
            if (mDeque.size() == mSize)
            {
                mDeque.pop_back();
                mDeque.push_front(item);
            }
            else
            {
                mDeque.push_front(item);
            }
        }
    }

    /* Returns true if the given item is in the cache. */
    bool contains(T item)
    {
        return (std::find(mDeque.begin(), mDeque.end(), item) != mDeque.end());
    }

    void erase(T item)
    {
        auto pos = std::find(mDeque.begin(), mDeque.end(), item);
        if (pos != mDeque.end())
            mDeque.erase(pos);
    }

    /* Returns a const iterator to the underlying data structure. */
    const std::deque<T>& iterator() const { return mDeque; }

    /* Returns true if the cache is empty */
    bool empty() { return mDeque.empty(); }

    /* Deletes all elements in the array */
    void clear() { return mDeque.clear(); }

  private:
    std::deque<T> mDeque;
    std::size_t mSize;
};

} // namespace ntmd