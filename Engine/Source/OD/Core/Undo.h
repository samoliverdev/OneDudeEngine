#pragma once
#include "OD/Base.h"
#include <deque>
#include <string>

namespace OD{

struct OD_API IUndoCommand {
    virtual void Undo() = 0;
    virtual void Redo() = 0;
    virtual ~IUndoCommand(){}
};

template<typename T>
class UndoValueCommand : public IUndoCommand {
public:
    UndoValueCommand(const std::string& label, T* target, T oldValue, T newValue)
        : label(label), target(target), oldValue(oldValue), newValue(newValue) {}

    void Undo() override {
        *target = oldValue;
    }

    void Redo() override {
        *target = newValue;
    }

private:
    std::string label;
    T* target;
    T oldValue;
    T newValue;
};

class OD_API UndoManager{
public:
    UndoManager(size_t maxSize): maxSize(maxSize){}

    UndoManager(const UndoManager&) = delete;
    UndoManager& operator=(const UndoManager&) = delete;
    UndoManager(UndoManager&&) = default;
    UndoManager& operator=(UndoManager&&) = default;

    static UndoManager& Get();

    void Execute(std::unique_ptr<IUndoCommand> command);
    void Undo();
    void Redo();
    void Clear();

    template<typename T>
    void RecordValue(const std::string& label, T* target, T newValue){
        T oldValue = *target;
        if(oldValue == newValue) return;
        Execute(std::make_unique<UndoValueCommand<T>>(label, target, oldValue, newValue));
    } 

private:
    size_t maxSize;
    std::deque<std::unique_ptr<IUndoCommand>> undoStack;
    std::deque<std::unique_ptr<IUndoCommand>> redoStack;

    inline void AddToStack(std::deque<std::unique_ptr<IUndoCommand>>& stack, std::unique_ptr<IUndoCommand> cmd){
        if(stack.size() >= maxSize) stack.pop_front();
        stack.push_back(std::move(cmd));
    }
};


}