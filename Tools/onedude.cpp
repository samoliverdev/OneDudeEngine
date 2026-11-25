#include "OD/Core/LiveCommandProtocol.h"
#include <algorithm>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <thread>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#else
#include <poll.h>
#include <signal.h>
#endif

namespace{
namespace fs = std::filesystem;
using namespace OD::LiveCommandProtocol;
using Clock = std::chrono::steady_clock;
using Arguments = std::vector<std::string>;

bool ParsePid(const std::string& value, unsigned long& pid){
    const auto result = std::from_chars(value.data(), value.data() + value.size(), pid);
    return result.ec == std::errc() && result.ptr == value.data() + value.size() && pid != 0;
}

bool EndpointExists(const fs::path& directory, unsigned long pid){
#ifdef _WIN32
    const auto name = PipeName(pid);
    for(int attempt = 0; attempt < 3; ++attempt){
        if(WaitNamedPipeW(name.c_str(), 1)) return true;
        const auto error = GetLastError();
        if(error == ERROR_SEM_TIMEOUT || error == ERROR_PIPE_BUSY) return true;
        if(error != ERROR_FILE_NOT_FOUND) break;
        Sleep(10);
    }
    return false;
#else
    if(pid > static_cast<unsigned long>(std::numeric_limits<pid_t>::max())) return false;
    const auto address = SocketAddress(directory, pid);
    struct stat info{};
    return address.sun_family == AF_UNIX && lstat(address.sun_path, &info) == 0 &&
        S_ISSOCK(info.st_mode) && kill(static_cast<pid_t>(pid), 0) == 0;
#endif
}

bool ContainsPath(const fs::path& directory, const fs::path& path){
    if(directory == path) return true;
    const auto relative = path.lexically_relative(directory);
    return !relative.empty() && *relative.begin() != "..";
}

unsigned long FindSession(const fs::path& directory){
    std::error_code error;
    const auto cwd = fs::weakly_canonical(fs::current_path(), error);
    struct Session{ unsigned long pid; fs::path project; };
    std::vector<Session> sessions;
    if(!directory.empty()){
        for(const auto& entry : fs::directory_iterator(directory)){
            if(entry.path().extension() != ".session" || !entry.is_regular_file(error) ||
                entry.file_size(error) > MaxMessage) continue;
            unsigned long pid;
            if(!ParsePid(entry.path().stem().string(), pid) || !EndpointExists(directory, pid)) continue;
            std::ifstream file(entry.path(), std::ios::binary);
            const std::string path{std::istreambuf_iterator<char>(file), {}};
            if(!path.empty()) sessions.push_back({pid, fs::u8path(path)});
        }
    }
    if(sessions.empty()){
        std::cerr << "No live editor found. Start your project with --live-commands.\n";
        return 0;
    }
    if(sessions.size() == 1) return sessions.front().pid;

    unsigned long match = 0;
    for(const auto& session : sessions){
        if(!ContainsPath(session.project, cwd) && !ContainsPath(cwd, session.project)) continue;
        if(match != 0){
            match = 0;
            break;
        }
        match = session.pid;
    }
    if(match != 0) return match;
    std::cerr << "Multiple live editors found. Use onedude --pid <pid> to choose a session.\n";
    for(const auto& session : sessions) std::cerr << session.pid << "  " << session.project.u8string() << '\n';
    return 0;
}

int PrintReply(const char* reply, size_t length){
    if(length == 0 || length > MaxMessage || (reply[0] != '0' && reply[0] != '1')){
        std::cerr << "Engine sent an invalid reply.\n";
        return 2;
    }
    auto& output = reply[0] == '0' ? std::cout : std::cerr;
    if(length > 1) output.write(reply + 1, length - 1) << '\n';
    return reply[0] == '0' ? 0 : 1;
}

#ifdef _WIN32
int Request(const fs::path&, unsigned long pid, const std::string& request){
    const auto name = PipeName(pid);
    const auto deadline = Clock::now() + std::chrono::seconds(60);
    const auto reconnectDeadline = Clock::now() + std::chrono::seconds(1);
    HANDLE pipe = INVALID_HANDLE_VALUE;
    while(Clock::now() < deadline){
        pipe = CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if(pipe != INVALID_HANDLE_VALUE) break;
        const auto error = GetLastError();
        if(error == ERROR_FILE_NOT_FOUND && Clock::now() < reconnectDeadline){
            Sleep(10);
            continue;
        }
        if(error != ERROR_PIPE_BUSY) break;
        if(!WaitNamedPipeW(name.c_str(), 1000) && GetLastError() != ERROR_SEM_TIMEOUT) break;
    }
    if(pipe == INVALID_HANDLE_VALUE){
        std::cerr << "Could not connect to engine PID " << pid << ".\n";
        return 2;
    }
    ULONG serverPid = 0;
    DWORD written = 0;
    if(GetNamedPipeServerProcessId(pipe, &serverPid) && serverPid == pid &&
        WriteFile(pipe, request.data(), static_cast<DWORD>(request.size()), &written, nullptr) &&
        written == request.size()){
        while(Clock::now() < deadline){
            DWORD available = 0;
            if(!PeekNamedPipe(pipe, nullptr, 0, nullptr, &available, nullptr)) break;
            if(available != 0){
                char reply[MaxMessage];
                DWORD length = 0;
                const bool read = ReadFile(pipe, reply, sizeof(reply), &length, nullptr);
                CloseHandle(pipe);
                return PrintReply(reply, read ? length : 0);
            }
            Sleep(10);
        }
    }
    CloseHandle(pipe);
    std::cerr << "Engine did not return a reply.\n";
    return 2;
}
#else
int Request(const fs::path& directory, unsigned long pid, const std::string& request){
    const auto address = SocketAddress(directory, pid);
    const int connection = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if(connection < 0){
        std::cerr << "Could not create client socket.\n";
        return 2;
    }
    const auto deadline = Clock::now() + std::chrono::seconds(60);
    bool connected = false;
    while(address.sun_family == AF_UNIX && Clock::now() < deadline){
        if(connect(connection, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0){
            connected = true;
            break;
        }
        if(errno != EAGAIN && errno != EINTR) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if(connected && VerifyPeer(connection, pid) &&
        send(connection, request.data(), request.size(), MSG_NOSIGNAL) == static_cast<ssize_t>(request.size())){
        while(Clock::now() < deadline){
            const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - Clock::now()).count();
            if(remaining <= 0) break;
            pollfd event{connection, POLLIN, 0};
            const int ready = poll(&event, 1, static_cast<int>(remaining));
            if(ready < 0 && errno == EINTR) continue;
            if(ready <= 0 || !(event.revents & POLLIN)) break;
            char reply[MaxMessage];
            const auto length = recv(connection, reply, sizeof(reply), MSG_TRUNC);
            close(connection);
            return PrintReply(reply, length > 0 ? static_cast<size_t>(length) : 0);
        }
    }
    close(connection);
    std::cerr << "Engine did not return a reply for PID " << pid << ".\n";
    return 2;
}
#endif

int Run(Arguments args){
    unsigned long pid = 0;
    if(args.size() >= 2 && args[0] == "--pid" && ParsePid(args[1], pid)){
        args.erase(args.begin(), args.begin() + 2);
    }
    if(args.empty() || args[0] == "--help"){
        std::cout << "Usage: onedude [--pid <pid>] <status | list [--json] | command <name> [args...]>\n";
        return args.empty() ? 2 : 0;
    }
    const auto directory = SessionDirectory();
    if(args[0] == "command" && args.size() >= 2) args.erase(args.begin());
    else if(args[0] != "status" && args[0] != "list"){
        std::cerr << "Use onedude --help for usage.\n";
        return 2;
    }
    if(args[0] == "capture-viewport" && args.size() == 2 && !args[1].empty()){
        args[1] = fs::absolute(fs::u8path(args[1])).lexically_normal().u8string();
    }
    if(pid == 0) pid = FindSession(directory);
    if(pid == 0) return 2;
    std::string request;
    for(const auto& arg : args){
        if(arg.size() + request.size() >= MaxMessage){
            std::cerr << "Command exceeds message limit.\n";
            return 2;
        }
        request += arg;
        request.push_back('\0');
    }
    return Request(directory, pid, request);
}
}

#ifdef _WIN32
int wmain(int argc, wchar_t** argv){
#else
int main(int argc, char** argv){
#endif
    try{
        Arguments args;
        for(int i = 1; i < argc; ++i) args.push_back(fs::path(argv[i]).u8string());
        return Run(std::move(args));
    } catch(const std::exception& error){
        std::cerr << error.what() << '\n';
        return 2;
    }
}
