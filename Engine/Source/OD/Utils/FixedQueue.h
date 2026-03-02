#pragma once

namespace OD{

template<typename T>
class FixedQueue{
public:
    FixedQueue(size_t inMaxSize):maxSize(inMaxSize){}

    void Push(const T& value){
        if(data.size() >= maxSize) data.erase(data.begin()); // remove oldest
        data.push_back(value); // add newest
    }

    T Back() const {
        if(data.empty()) return T{}; // return default (nullptr for shared_ptr)
        return data.back(); // return by value
    }

    bool Empty() const {
        return data.empty();
    }

    const std::vector<T>& Data() const {
        return data;
    }

private:
    size_t maxSize;
    std::vector<T> data;
};

}