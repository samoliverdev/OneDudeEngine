#include "OD/pch.h"
#include "LiveCommands.h"
#include "LiveCommandProtocol.h"
#include "Log.h"
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <map>
#include <memory>
#include <string_view>
#include <cereal/external/rapidjson/writer.h>
#include <cereal/external/rapidjson/stringbuffer.h>

#ifdef _WIN32
#include <Windows.h>
#include <sddl.h>
#endif

namespace OD{
namespace{
struct Command{
    const Module* owner;
    LiveCommands::Handler handler;
};
std::map<std::string, Command> commands;
using Clock = std::chrono::steady_clock;
using LiveCommandProtocol::MaxMessage;
Clock::time_point deadline;
std::shared_ptr<std::string> response;
std::filesystem::path sessionFile;

void PublishSession(const std::filesystem::path& directory, unsigned long pid){
    if(directory.empty()) return;
    const auto file = directory / (std::to_string(pid) + ".session");
    auto temporary = file;
    temporary += ".tmp";
    std::ofstream output(temporary, std::ios::binary);
    output << std::filesystem::current_path().parent_path().u8string();
    output.close();
    std::error_code error;
    if(output) std::filesystem::rename(temporary, file, error);
    if(output && !error) sessionFile = file;
    else{
        LogError("Could not publish the live command session");
        std::filesystem::remove(temporary, error);
    }
}

#ifdef _WIN32
HANDLE pipe = INVALID_HANDLE_VALUE;
enum class Connection{ Listening, Reading, Waiting, ReplySent };
Connection connection = Connection::Listening;

void ResetConnection(){
    DisconnectNamedPipe(pipe);
    response.reset();
    connection = Connection::Listening;
}

PSECURITY_DESCRIPTOR CurrentUserDescriptor(){
    HANDLE token;
    if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return nullptr;
    DWORD required = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &required);
    std::vector<unsigned char> user(required);
    const bool loaded = required != 0 && GetTokenInformation(token, TokenUser, user.data(), required, &required);
    CloseHandle(token);
    if(!loaded) return nullptr;

    LPWSTR sid = nullptr;
    if(!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(user.data())->User.Sid, &sid)) return nullptr;
    const auto sddl = std::wstring(L"D:P(A;;GA;;;") + sid + L")";
    LocalFree(sid);
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &descriptor, nullptr);
    return descriptor;
}
#elif defined(__linux__) && !defined(__ANDROID__)
int listener = -1;
int client = -1;
std::filesystem::path socketPath;

void ResetConnection(){
    close(client);
    client = -1;
    response.reset();
}
#endif

void Dispatch(const char* data, size_t size){
    response = std::make_shared<std::string>();
    LiveCommands::Reply reply = [pending = std::weak_ptr<std::string>(response)](bool success, std::string message){
        if(auto result = pending.lock(); result && result->empty()){
            if(message.size() >= MaxMessage) *result = "1Response exceeds message limit";
            else *result = (success ? "0" : "1") + message;
        }
    };
    if(size == 0 || data[size - 1] != '\0'){
        reply(false, "Malformed command");
        return;
    }
    LiveCommands::Arguments args;
    for(size_t offset = 0; offset < size;){
        args.emplace_back(data + offset);
        offset += args.back().size() + 1;
    }
    std::string name = std::move(args.front());
    args.erase(args.begin());
    if(name == "help" || name == "list"){
        if(args.size() == 1 && args[0] == "--json"){
            CEREAL_RAPIDJSON_NAMESPACE::StringBuffer buffer;
            CEREAL_RAPIDJSON_NAMESPACE::Writer<CEREAL_RAPIDJSON_NAMESPACE::StringBuffer> json(buffer);
            json.StartObject();
            json.Key("commands");
            json.StartArray();
            json.String("help");
            json.String("list");
            for(const auto& entry : commands) json.String(entry.first.c_str());
            json.EndArray();
            json.EndObject();
            reply(true, buffer.GetString());
            return;
        }
        if(!args.empty()){
            reply(false, "Usage: list [--json]");
            return;
        }
        std::string list = "help";
        if(name == "list") list += "\nlist";
        for(const auto& entry : commands) list += "\n" + entry.first;
        reply(true, std::move(list));
        return;
    }
    auto found = commands.find(name);
    if(found == commands.end()){
        reply(false, "Unknown command: " + name);
        return;
    }
    try{
        found->second.handler(args, reply);
    } catch(const std::exception& error){
        reply(false, error.what());
    } catch(...){
        reply(false, "Command failed");
    }
}
}

bool LiveCommands::Register(Module& owner, std::string name, Handler handler){
    if(name.empty() || name == "help" || name == "list" || !handler) return false;
    return commands.emplace(std::move(name), Command{&owner, std::move(handler)}).second;
}

void LiveCommands::Unregister(const Module& owner){
    for(auto it = commands.begin(); it != commands.end();){
        if(it->second.owner == &owner) it = commands.erase(it);
        else ++it;
    }
}

void LiveCommands::Start(bool requested){
#ifdef _WIN32
    wchar_t enabled[2]{};
    if(!requested && (GetEnvironmentVariableW(L"OD_LIVE_COMMANDS", enabled, 2) != 1 || enabled[0] != L'1')) return;
    auto descriptor = CurrentUserDescriptor();
    if(!descriptor){
        LogError("Could not configure live command pipe permissions");
        return;
    }
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), descriptor, FALSE};
    const DWORD pid = GetCurrentProcessId();
    pipe = CreateNamedPipeW(LiveCommandProtocol::PipeName(pid).c_str(),
        PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_NOWAIT | PIPE_REJECT_REMOTE_CLIENTS,
        1, MaxMessage, MaxMessage, 0, &security);
    const DWORD error = GetLastError();
    LocalFree(descriptor);
    if(pipe == INVALID_HANDLE_VALUE) LogError("Could not open live command pipe (Windows error {})", error);
    else{
        PublishSession(LiveCommandProtocol::SessionDirectory(), pid);
        LogInfo("Live commands enabled for PID {}", pid);
    }
#elif defined(__linux__) && !defined(__ANDROID__)
    const char* enabled = std::getenv("OD_LIVE_COMMANDS");
    if(!requested && (!enabled || std::string_view(enabled) != "1")) return;
    const auto directory = LiveCommandProtocol::SessionDirectory();
    const auto pid = static_cast<unsigned long>(getpid());
    const auto address = LiveCommandProtocol::SocketAddress(directory, pid);
    if(address.sun_family != AF_UNIX){
        LogError("Could not create a private live command session directory");
        return;
    }
    listener = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if(listener < 0){
        LogError("Could not create live command socket");
        return;
    }
    unlink(address.sun_path);
    const bool bound = bind(listener, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0;
    if(!bound || listen(listener, 4) != 0){
        if(bound) unlink(address.sun_path);
        close(listener);
        listener = -1;
        LogError("Could not open live command socket");
        return;
    }
    socketPath = address.sun_path;
    PublishSession(directory, pid);
    LogInfo("Live commands enabled for PID {}", pid);
#endif
}

void LiveCommands::Poll(){
#ifdef _WIN32
    if(pipe == INVALID_HANDLE_VALUE) return;
    if(connection == Connection::Listening){
        // A nonzero result on a nonblocking pipe only arms the listener.
        if(ConnectNamedPipe(pipe, nullptr)) return;
        const DWORD error = GetLastError();
        if(error == ERROR_NO_DATA) ResetConnection();
        if(error != ERROR_PIPE_CONNECTED) return;
        deadline = Clock::now() + std::chrono::seconds(65);
        connection = Connection::Reading;
    }
    if(Clock::now() >= deadline || !PeekNamedPipe(pipe, nullptr, 0, nullptr, nullptr, nullptr)){
        ResetConnection();
        return;
    }
    if(connection == Connection::Reading){
        char buffer[MaxMessage];
        DWORD size = 0;
        if(!ReadFile(pipe, buffer, sizeof(buffer), &size, nullptr)){
            if(GetLastError() != ERROR_NO_DATA) ResetConnection();
            return;
        }
        connection = Connection::Waiting;
        Dispatch(buffer, size);
    }
    if(connection == Connection::Waiting && response && !response->empty()){
        DWORD written = 0;
        if(!WriteFile(pipe, response->data(), static_cast<DWORD>(response->size()), &written, nullptr)){
            ResetConnection();
        } else if(written != 0){
            connection = Connection::ReplySent;
        }
    }
#elif defined(__linux__) && !defined(__ANDROID__)
    if(listener < 0) return;
    if(client < 0){
        client = accept4(listener, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if(client < 0) return;
        if(!LiveCommandProtocol::VerifyPeer(client)){
            ResetConnection();
            return;
        }
        deadline = Clock::now() + std::chrono::seconds(65);
    }
    if(Clock::now() >= deadline){
        ResetConnection();
        return;
    }
    char buffer[MaxMessage];
    const auto size = recv(client, buffer, sizeof(buffer), MSG_TRUNC | (response ? MSG_PEEK : 0));
    if(size == 0 || size > static_cast<ssize_t>(MaxMessage) ||
        (size < 0 && errno != EAGAIN && errno != EINTR)){
        ResetConnection();
        return;
    }
    if(!response && size > 0) Dispatch(buffer, static_cast<size_t>(size));
    if(response && !response->empty()){
        const auto written = send(client, response->data(), response->size(), MSG_NOSIGNAL);
        if(written >= 0 || (errno != EAGAIN && errno != EINTR)) ResetConnection();
    }
#endif
}

void LiveCommands::Stop(){
#ifdef _WIN32
    if(pipe != INVALID_HANDLE_VALUE){
        ResetConnection();
        CloseHandle(pipe);
        pipe = INVALID_HANDLE_VALUE;
    }
#elif defined(__linux__) && !defined(__ANDROID__)
    if(client >= 0) ResetConnection();
    if(listener >= 0){
        close(listener);
        listener = -1;
        unlink(socketPath.c_str());
    }
#endif
    if(!sessionFile.empty()){
        std::error_code error;
        std::filesystem::remove(sessionFile, error);
        sessionFile.clear();
    }
    commands.clear();
}

}
