#pragma once
#include <vector>
#include <functional>
#include <utility>

namespace OD{

template<class Sig>
class Action{
public:
    template<typename Functor>
    void Add(Functor&& f){ 
        _funcs.emplace_back(std::forward<Functor>(f)); 
    }

    template<class... Args>
    void Invoke(Args&&... args) const{
        for(auto& f : _funcs) f(args...);
    }

    void Clean(){
        _funcs.clear();
    }

private:
    std::vector<std::function<Sig>> _funcs;
    //std::vector<Sig> _funcs;
};

}