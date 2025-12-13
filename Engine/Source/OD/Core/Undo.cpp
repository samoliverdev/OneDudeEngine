#include "OD/pch.h"
#include "Undo.h"

namespace OD{

UndoManager& UndoManager::Get(){
    static UndoManager undoManager(50);
    return undoManager;
}

void UndoManager::Execute(std::unique_ptr<IUndoCommand> command){
    command->Redo();
    AddToStack(undoStack, std::move(command));
    redoStack.clear();
}

void UndoManager::Undo(){
    if(undoStack.empty()) return;
    auto command = std::move(undoStack.back());
    undoStack.pop_back();
    command->Undo();
    AddToStack(redoStack, std::move(command));
}

void UndoManager::Redo(){
    if(redoStack.empty()) return;
    auto command = std::move(redoStack.back());
    redoStack.pop_back();
    command->Redo();
    AddToStack(undoStack, std::move(command));
}

void UndoManager::Clear(){
    undoStack.clear();
    redoStack.clear();
}

}