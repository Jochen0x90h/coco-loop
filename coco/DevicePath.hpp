#pragma once

#include <filesystem>


namespace coco {

class DevicePath {
#ifdef _WIN32
    using PathType = const wchar_t*;
#else
    using PathType = const char*;
#endif

public:
    DevicePath(PathType path) : path_(path) {}

    PathType c_str() const {return path_;}
    std::filesystem::path path() const {return path_;}

protected:
    PathType path_;
};

template <typename S>
S &operator <<(S &stream, const DevicePath &path) {
#ifdef _WIN32
    return stream << path.path().string();
#else
    return stream << path.c_str();
#endif
}

} // namespace coco
