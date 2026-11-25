#pragma once
#include <string>
#include <filesystem>

#if defined(__linux__) && !defined(__ANDROID__)
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#endif

namespace OD::LiveCommandProtocol{

inline constexpr unsigned int MaxMessage = 16 * 1024;

inline std::filesystem::path SessionDirectory(){
    std::error_code error;
#ifdef _WIN32
    auto directory = std::filesystem::temp_directory_path(error) / "OneDudeEngine-sessions";
    if(error) return {};
    std::filesystem::create_directory(directory, error);
    return error ? std::filesystem::path{} : directory;
#elif defined(__linux__) && !defined(__ANDROID__)
    auto directory = std::filesystem::path("/tmp") / ("onedude-" + std::to_string(getuid()));
    mkdir(directory.c_str(), 0700);
    struct stat info{};
    if(lstat(directory.c_str(), &info) != 0 || !S_ISDIR(info.st_mode) ||
       info.st_uid != getuid() || (info.st_mode & 077) != 0) return {};
    return directory;
#else
    return {};
#endif
}

#if defined(__linux__) && !defined(__ANDROID__)
inline sockaddr_un SocketAddress(const std::filesystem::path& directory, unsigned long pid){
    sockaddr_un address{};
    const auto path = (directory / (std::to_string(pid) + ".sock")).string();
    if(directory.empty() || path.size() >= sizeof(address.sun_path)) return address;
    address.sun_family = AF_UNIX;
    std::memcpy(address.sun_path, path.c_str(), path.size() + 1);
    return address;
}

inline bool VerifyPeer(int socket, unsigned long pid = 0){
    struct ucred peer{};
    socklen_t size = sizeof(peer);
    return getsockopt(socket, SOL_SOCKET, SO_PEERCRED, &peer, &size) == 0 &&
        peer.uid == getuid() && (pid == 0 || static_cast<unsigned long>(peer.pid) == pid);
}
#endif

inline std::wstring PipeName(unsigned long pid){
    return LR"(\\.\pipe\OneDudeEngine-)" + std::to_wstring(pid);
}

}
