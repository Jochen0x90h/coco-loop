#include <coco/loop.hpp>
#include <coco/debug.hpp>


using namespace coco;


Coroutine timer1() {
	while (true) {
		debug::setRed(true);
		co_await loop::sleep(100ms);

		debug::setRed(false);
		co_await loop::sleep(1900ms);
	}
}

Coroutine timer2() {
	while (true) {
		debug::toggleGreen();

		auto timeout = loop::now() + 3s;
		co_await loop::sleep(timeout);

		// test if sleep with elapsed timeout works
		co_await loop::sleep(timeout);
	}
}

Coroutine timer3() {
	while (true) {
		debug::toggleBlue();
		
		// test yield
		co_await loop::yield();

		// test if time overflow works on nrf52
		auto time = loop::now();
		int i = int(time.value >> 20) & 3;

		co_await loop::sleep(500ms + i * 1s);
	}
}

int main() {
	debug::init();

	timer1();
	timer2();
	timer3();

	loop::run();
}
