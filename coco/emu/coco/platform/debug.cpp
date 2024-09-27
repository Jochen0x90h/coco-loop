#include <coco/debug.hpp>
#include <chrono>
#include <iostream>
#include <thread>
//#include <locale>
//#include <codecvt>


namespace coco {
namespace debug {

bool red = false;
bool green = false;
bool blue = false;

void init() {}

inline void set(bool &b, bool value, bool function) {
    if (function)
        b = value;
    else if (value)
        b = !b;
}

void set(uint32_t bits, uint32_t function) {
    set(red, (bits & 1) != 0, (function & 1) != 0);
    set(green, (bits & 2) != 0, (function & 2) != 0);
    set(blue, (bits & 4) != 0, (function & 4) != 0);
}

void sleep(Microseconds<> time) {
    std::this_thread::sleep_for(std::chrono::microseconds(time.value));
}

void write(const char *message, int length) {
    int start = 0;
    for (int i = 0; i <= length; ++i) {
        if (i == length || message[i] == '\n') {
            //std::wstring s;
            //std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> conv;
            //std::wstring wstr = conv.from_bytes(message + start, message + i);

            std::cout.write(message + start, i - start);
            //std::wcout << wstr;
            if (i == length)
                return;
            std::cout << std::endl;
            //std::wcout << std::endl;
            start = i + 1;
            if (start == length)
                return;
        }
    }
}

} // namespace debug
} // namespace coco
