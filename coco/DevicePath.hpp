#pragma once

#include <filesystem>


namespace coco {

/// @brief Path to a device on an operating system.
/// Only references the data, use std::filesystem::path to store a device path in memory.
/// This class prevents copying the path data in callbacks, i.e. is a "path reference".
class DevicePath {
public:
    using value_type = std::filesystem::path::value_type;

    /// @brief Constructor from null terminated string.
    /// Keeps a pointer to the data.
    /// @param path Null-terminated string (const char * or const wchar_t *)
    DevicePath(const value_type *path) : path_(path) {}

    /// @brief Constructor from std::filesystem::path.
    /// Uses path.c_str() and keeps a pointer to the data
    /// @param path Path
    DevicePath(const std::filesystem::path &path) : path_(path.c_str()) {}

    const value_type *c_str() const {return path_;}
    std::filesystem::path path() const {return path_;}

protected:
    const value_type *path_;
};


inline bool operator < (const std::filesystem::path &a, const DevicePath &b) {
    std::basic_string_view<std::filesystem::path::value_type> sv(b.c_str());
    return a.compare(sv) < 0;
}
inline bool operator < (const DevicePath &a, const std::filesystem::path &b) {
    std::basic_string_view<std::filesystem::path::value_type> sv(a.c_str());
    return b.compare(sv) > 0;
}


template <typename S>
S &operator <<(S &stream, const DevicePath &path) {
#ifdef _WIN32
    return stream << path.path().string();
#else
    return stream << path.c_str();
#endif
}

} // namespace coco
