#pragma once
#include "OD/Defines.h"
#include <functional>
#include <string>
#include <vector>

namespace OD{

class Module;

class OD_API LiveCommands{
public:
    using Arguments = std::vector<std::string>;
    using Reply = std::function<void(bool success, std::string message)>;
    using Handler = std::function<void(const Arguments&, Reply)>;

    // Register from module lifecycle callbacks. Commands and replies run on
    // the application thread. A handler may retain its reply across frames.
    static bool Register(Module& owner, std::string name, Handler handler);

private:
    friend class Application;
    static void Start(bool requested);
    static void Poll();
    static void Stop();
    static void Unregister(const Module& owner);
};

}
