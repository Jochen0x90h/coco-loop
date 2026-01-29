#include <coco/convert.hpp>
#include <coco/debug.hpp>
#include <LoopTest.hpp>


using namespace coco;


Coroutine redTask(Loop &loop) {
    while (true) {
        debug::setRed(true);
        co_await loop.sleep(100ms);

        debug::setRed(false);
        co_await loop.sleep(1900ms);

        debug::out << "red 2s\n";
    }
}

Coroutine greenTask(Loop &loop) {
    while (true) {
        debug::toggleGreen();

        auto timeout = loop.now() + 1s;
        co_await loop.sleep(timeout);

        // test if sleep with elapsed timeout works
        co_await loop.sleep(timeout);

        debug::out << "green 1s\n";
    }
}

Coroutine blueTask(Loop &loop) {
    while (true) {
        debug::toggleBlue();

        // test if time overflow works on nrf52
        auto time = loop.now();
        int i = int(time.value >> 20) & 3;

        Loop::Duration duration = 500ms + i * 1s;
        co_await loop.sleep(duration);

        // test yield
        co_await loop.yield();

        debug::out << "blue " << dec(int(duration / 1ms)) << "ms\n";
    }
}

int main() {
    debug::out << "LoopTest\n";

    redTask(drivers.loop);
    greenTask(drivers.loop);
    blueTask(drivers.loop);

    drivers.loop.run();
}
