#include "Loop_RTC0.hpp"
#include <coco/platform/platform.hpp>


namespace coco {

// interval of 24 bit timer is 1024 seconds (2^24 / 16384Hz), given in milliseconds
constexpr int INTERVAL = 1024000;


Loop_RTC0::~Loop_RTC0() {
}

void Loop_RTC0::run(const int &condition) {
	int c = condition;
	while (c == condition) {
		// resume coroutines waiting on yield()
		this->yieldTaskList.resumeAll();

		// resume coroutines waiting on sleep()
		// todo: keep sleepWaitlist in sorted order as optimization
		Time currentTime = now();
		this->sleepTaskList.resumeAll([currentTime](Time time) {
			// check if this time has elapsed
			return time <= currentTime;
		});

		// get sleep time
		Time sleepTime = {currentTime.value + INTERVAL - 1};
		this->sleepTaskList.visitAll([&sleepTime](Time time) {
			// check if this time is the next to elapse
			if (time < sleepTime)
				sleepTime = time;
		});

		// only sleep if there are no coroutines waiting on yield()
		if (this->yieldTaskList.empty()) {
			// set new timeout and clear pending interrupt flags at peripheral and NVIC
			NRF_RTC0->CC[0] = ((sleepTime.value - this->baseTime) << (7 + 4)) / 125;
			NRF_RTC0->EVENTS_COMPARE[0] = 0;
			clearInterrupt(RTC0_IRQn);

			// wait for event
			// see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0321a/BIHICBGB.html
			if (now() < sleepTime) {
				//debug::toggleRed();

				// data synchronization barrier
				__DSB();

				// wait for event (interrupts trigger an event due to SEVONPEND)
				__WFE();
			}
		}

		// call handlers
		auto it = this->handlers.begin();
		while (it != this->handlers.end()) {
			// increment iterator beforehand because a handler can remove() itself
			auto current = it;
			++it;
			current->handle();
		}
	}
}

Awaitable<> Loop_RTC0::yield() {
	return {this->yieldTaskList};
}

Time Loop_RTC0::now() {
	// time resolution 1/1000 s
	uint32_t counter = NRF_RTC0->COUNTER;
	if (NRF_RTC0->EVENTS_OVRFLW) {
		NRF_RTC0->EVENTS_OVRFLW = 0;

		// reload counter in case overflow happened after reading the counter
		counter = NRF_RTC0->COUNTER;

		// advance base time by one interval (1024 seconds)
		this->baseTime += INTERVAL;
	}
	return {this->baseTime + ((counter * 125 + 1024) >> (7 + 4))};
}

Awaitable<Time> Loop_RTC0::sleep(Time time) {
	// todo: insert into waitlist in sorted order
	return {this->sleepTaskList, time};
}
/*
void sleepBlocking(int us) {
	auto cycles = us * 8;
	for (int i = 0; i < cycles; ++i) {
		__NOP();
	}
}*/


// Loop_RTC0

Loop_RTC0::Handler::~Handler() {
}

} // namespace coco
