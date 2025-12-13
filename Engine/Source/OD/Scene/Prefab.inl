namespace OD{

template<typename T> 
bool Prefab::HasComponentInRoot(){
    Assert(scene != nullptr);
    Assert(root != EntityNull);
    return scene->HasComponent<T>(root);
}

template<typename T> 
T& Prefab::GetComponentInRoot(){
    Assert(scene != nullptr);
    Assert(root != EntityNull);
    return scene->GetComponent<T>(root);
}

template<typename T> 
T* Prefab::TryGetComponentInRoot(){
    Assert(scene != nullptr);
    Assert(root != EntityNull);
    return scene->TryGetComponent<T>(root);
}


}