#include <coco/loop.hpp>
#include <coco/debug.hpp>
#include <LoopTest.hpp>


using namespace coco;


Coroutine timer1(Loop &loop) {
	while (true) {
		debug::setRed(true);
		co_await loop.sleep(100ms);

		debug::setRed(false);
		co_await loop.sleep(1900ms);
	}
}

Coroutine timer2(Loop &loop) {
	while (true) {
		debug::toggleGreen();

		auto timeout = loop.now() + 3s;
		co_await loop.sleep(timeout);

		// test if sleep with elapsed timeout works
		co_await loop.sleep(timeout);
	}
}

Coroutine timer3(Loop &loop) {
	while (true) {
		debug::toggleBlue();
		
		// test yield
		co_await loop.yield();

		// test if time overflow works on nrf52
		auto time = loop.now();
		int i = int(time.value >> 20) & 3;

		co_await loop.sleep(500ms + i * 1s);
	}
}

int main() {
	Drivers drivers;
	debug::init();

	timer1(drivers.loop);
	timer2(drivers.loop);
	timer3(drivers.loop);

	drivers.loop.run();
}
