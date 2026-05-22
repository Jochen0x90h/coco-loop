#pragma once

#ifdef _WIN32
#include "Loop_Win32.hpp"
namespace coco {
using Loop_native = Loop_Win32;
}
#endif

#ifdef __linux__
#include "Loop_io_uring.hpp"
namespace coco {
using Loop_native = Loop_io_uring;
}
#endif
