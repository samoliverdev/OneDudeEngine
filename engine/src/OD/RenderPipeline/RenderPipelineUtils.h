#pragma once

namespace OD{

template<typename Key, typename Value>
struct CommandBucket1{
    std::vector<std::pair<Key, Value>> commands;
    std::function<bool(std::pair<Key, Value>&,std::pair<Key, Value>&)> sortFunction = nullptr;

    inline void Clear(){
        commands.clear();
    }

    inline void Add(Key k, Value v){
        commands.push_back(std::make_pair(k, v));
    }

    inline void Sort(){
        if(sortFunction == nullptr){
            Assert(false && "No SortFunction");
            return;
        }

        /*std::sort(commands.begin(), commands.end(), [](const auto& a, const auto& b){
            return (a.first < b.first);
        });*/
        
        if(sortFunction != nullptr){
            std::sort(commands.begin(), commands.end(), sortFunction);
        } else {
            std::sort(commands.begin(), commands.end());
        }
    }

    inline void Each(std::function<void(Value& value)> func){
        for(auto i: commands){
            func(i.second);
        }
    }
};

template<typename Key, typename Value>
struct CommandBucket2{
    std::vector<std::pair<Key, Value>> commands;
    int count;

    inline void Clear(){
        count = 0;
    }

    inline void Add(Key k, Value v){
        if(commands.size() <= count){
            commands.push_back(std::make_pair(k, v));
        } else {
            commands[count] = std::make_pair(k, v);
        }

        count += 1;
    }

    inline void Sort(){
        std::sort(commands.begin(), commands.end());
    }

    inline void Each(std::function<void(Value& value)> func){
        int index = 0;
        for(auto i: commands){
            if(index >= count) break;
            func(i.second);
            index += 1;
        }
    }
};

template<typename Key, typename Value>
struct CommandBucket3{
    std::unordered_map<Key, std::vector<Value>> commands;
    //int count;

    inline void Clear(){
        commands.clear();
    }

    inline void Add(Key k, Value v){
        commands[k].push_back(v);
    }

    inline void Sort(){
        //std::sort(commands.begin(), commands.end());
    }

    inline void Each(std::function<void(Value& value)> func){
        for(auto i: commands){
            for(auto j: i.second){
                func(j);
            }
        }
    }
};

template<typename Key, typename Key2, typename Value>
struct CommandBucket4{
    std::unordered_map<Key, std::unordered_map<Key2, Value> > commands;

    inline void Clear(){
        commands.clear();
    }

    inline Value& Get(Key k, Key2 k2){
        return commands[k][k2];
    }

    inline void Sort(){
        //std::sort(commands.begin(), commands.end());
    }

    inline void Each(std::function<void(Value& value)> func){
        for(auto i: commands){
            for(auto j: i.second){
                func(j.second);
            }
        }
    }
};


}