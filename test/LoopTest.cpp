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


int i1;
int i2() {return i1;}
int &i3() {return i1;}
const int &i4() {return i1;}

enum class Foo {BAR};
Foo f1;
Foo f2() {return f1;}
Foo &f3() {return f1;}
const Foo &f4() {return f1;}

int main() {
	debug::init();
	Drivers drivers;

	timer1(drivers.loop);
	timer2(drivers.loop);
	timer3(drivers.loop);

	drivers.loop.run();

	// compilation checks for Loop::run(condition)
	drivers.loop.run(i1);
	//drivers.loop.run(i2()); // should not compile for r-value
	drivers.loop.run(i3());
	drivers.loop.run(i4());

	drivers.loop.run(f1);
	//drivers.loop.run(f2()); // should not compile for r-value
	drivers.loop.run(f3());
	drivers.loop.run(f4());
}
