#include "Loop_RTC0.hpp"
//#include <coco/debug.hpp>
#include <coco/platform/platform.hpp>
#include <coco/platform/nvic.hpp>


namespace coco {

// interval of 24 bit timer is 1024 seconds (2^24 / 16384Hz), given in milliseconds
constexpr int INTERVAL = 1024000;
constexpr int MAX_SLEEP = 1000000;


Loop_RTC0::Loop_RTC0(Mode mode) : mode(mode) {
	// disabled interrupts trigger an event and wake up the processor from WFE
	if (mode == Mode::WAIT)
		SCB->SCR = SCB->SCR | SCB_SCR_SEVONPEND_Msk;

	// initialize RTC0 to 16384Hz
	NRF_RTC0->EVTENSET = N(RTC_EVTENSET_OVRFLW, Set);
	NRF_RTC0->INTENSET = N(RTC_INTENSET_COMPARE0, Set);
	NRF_RTC0->PRESCALER = 1; // 16384Hz
	NRF_RTC0->CC[0] = MAX_SLEEP / 1000 * 16384;
	NRF_RTC0->TASKS_START = TRIGGER;
}

Loop_RTC0::~Loop_RTC0() {
}

void Loop_RTC0::run(const int &condition) {
	int c = condition;
	Time currentTime = now();
	while (c == condition) {
		// get sleep time
		Time sleepTime = this->sleepTasks2.getFirstTime(this->sleepTasks1.getFirstTime(currentTime + Duration::max() / 2));

		// set new timeout and clear pending interrupt flags at peripheral and NVIC
		int timeout = ((sleepTime.value - this->baseTime) << (7 + 4)) / 125 + 1; // sleep for one count longer ...
		NRF_RTC0->CC[0] = timeout;
		NRF_RTC0->EVENTS_COMPARE[0] = 0;
		nvic::clear(RTC0_IRQn);
		bool notPassed = int32_t((timeout - NRF_RTC0->COUNTER) << 8) > (1 << 8); // ... because we have to treat the next count as "passed"

		// wait for event if sleep time has not yet passed
		// see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0321a/BIHICBGB.html
		if (this->mode == Mode::WAIT && notPassed) {
			//debug::toggleRed();

			// data synchronization barrier
			__DSB();

			// wait for event (interrupts trigger an event due to SEVONPEND)
			__WFE();
		}

		// call handlers
		auto it = this->handlers.begin();
		while (it != this->handlers.end()) {
			// increment iterator beforehand because a handler can remove() itself
			auto current = it;
			++it;
			current->handle();
		}

		// resume coroutines waiting on sleep()
		currentTime = now();
		this->sleepTasks1.doUntil(currentTime);
		this->sleepTasks2.doUntil(currentTime);
	}
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
	return {this->baseTime + ((counter * 125) >> (7 + 4))};
}

Awaitable<CoroutineTimedTask> Loop_RTC0::sleep(Time time) {
	return {this->sleepTasks2, time};
}

/*
void sleepBlocking(int us) {
	auto cycles = us * 8;
	for (int i = 0; i < cycles; ++i) {
		__NOP();
	}
}*/

} // namespace coco
