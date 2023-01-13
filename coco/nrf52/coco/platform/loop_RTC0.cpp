#include <coco/loop.hpp>
#include <coco/platform/Handler.hpp>
#include <coco/platform/platform.hpp>
//#include <coco/debug.hpp>


/*
	Documentation:
		https://infocenter.nordicsemi.com/topic/struct_nrf52/struct/nrf52840.html

	Dependencies:

	Resources:
		NRF_RTC0
			CC[0]
*/
namespace coco {
namespace loop {

// interval of 24 bit timer is 1024 seconds (2^24 / 16384Hz), given in milliseconds
constexpr int INTERVAL = 1024000;

// base time for now() because the RTC counter is only 24 bit and runs at 16384Hz
uint32_t baseTime = 0;

// coroutines waiting on yield()
Waitlist<> yieldWaitlist;

// coroutines waiting on sleep()
Waitlist<Time> sleepWaitlist;


void run() {
	while (true) {
		// resume coroutines waiting on yield()
		loop::yieldWaitlist.resumeAll();

		// resume coroutines waiting on sleep()
		// todo: keep sleepWaitlist in sorted order as optimization
		Time currentTime = loop::now();
		loop::sleepWaitlist.resumeAll([currentTime](Time time) {
			// check if this time has elapsed
			return time <= currentTime;
		});

		// get sleep time
		Time sleepTime = {currentTime.value + INTERVAL - 1};
		loop::sleepWaitlist.visitAll([&sleepTime](Time time) {
			// check if this time is the next to elapse
			if (time < sleepTime)
				sleepTime = time;
		});

		// only sleep if there are no coroutines waiting on yield()
		if (loop::yieldWaitlist.empty()) {
			// set new timeout and clear pending interrupt flags at peripheral and NVIC
			NRF_RTC0->CC[0] = ((sleepTime.value - loop::baseTime) << (7 + 4)) / 125;
			NRF_RTC0->EVENTS_COMPARE[0] = 0;
			clearInterrupt(RTC0_IRQn);

			// wait for event
			// see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0321a/BIHICBGB.html
			if (loop::now() < sleepTime) {
				//debug::toggleRed();

				// data synchronization barrier
				__DSB();
			
				// wait for event (interrupts trigger an event due to SEVONPEND)
				__WFE();
			}
		}

		// call handlers
		auto it = coco::handlers.begin();
		while (it != coco::handlers.end()) {
			// increment iterator beforehand because a handler can remove() itself
			auto current = it;
			++it;
			current->handle();
		}
	}
}

Awaitable<> yield() {
	return {loop::yieldWaitlist};
}

Time now() {
	// time resolution 1/1000 s
	uint32_t counter = NRF_RTC0->COUNTER;
	if (NRF_RTC0->EVENTS_OVRFLW) {
		NRF_RTC0->EVENTS_OVRFLW = 0;

		// reload counter in case overflow happened after reading the counter
		counter = NRF_RTC0->COUNTER;

		// advance base time by one interval (1024 seconds)
		loop::baseTime += INTERVAL;
	}
	return {loop::baseTime + ((counter * 125 + 1024) >> (7 + 4))};
}

Awaitable<Time> sleep(Time time) {
	// todo: insert into waitlist in sorted order
	return {loop::sleepWaitlist, time};
}

void sleepBlocking(int us) {
	auto cycles = us * 8;
	for (int i = 0; i < cycles; ++i) {
		__NOP();
	}
}

} // namespace loop
} // namespace coco
