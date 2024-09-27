#include "Loop_RTC0.hpp"
//#include <coco/debug.hpp>
#include <coco/platform/nvic.hpp>


namespace coco {

// interval of 24 bit timer is 1024 seconds (2^24 / 16384Hz), given in milliseconds
constexpr int INTERVAL = 1024000;

// maximum sleep time is 500 seconds
constexpr int MAX_SLEEP = 500000;


Loop_RTC0::Loop_RTC0(Mode mode) : mode(mode) {
	// disabled interrupts trigger an event and wake up the processor from WFE
	if (mode == Mode::WAIT)
		SCB->SCR = SCB->SCR | SCB_SCR_SEVONPEND_Msk;

	// initialize RTC0 to 16384Hz
	NRF_RTC0->PRESCALER = 1; // 16384Hz
	//NRF_RTC0->CC[0] = MAX_SLEEP / 1000 * 16384;
	NRF_RTC0->EVTENSET = N(RTC_EVTENSET_OVRFLW, Set);
	NRF_RTC0->INTENSET = N(RTC_INTENSET_COMPARE0, Set);
	NRF_RTC0->TASKS_START = TRIGGER;
}

Loop_RTC0::~Loop_RTC0() {
}

void Loop_RTC0::run() {
	Time currentTime = now();
	while (!this->exitFlag) {
		// get sleep time (point in time when the first task is due)
		Time sleepTime = this->sleepTasks2.getFirstTime(this->sleepTasks1.getFirstTime(currentTime + MAX_SLEEP * 1ms/*Duration::max() / 2*/));

		// wait for event if sleep time has not yet passed
		// see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0321a/BIHICBGB.html
		if (this->mode == Mode::WAIT) {
			// set new timeout and clear pending interrupt flags at peripheral and NVIC
			int32_t timeout = ((sleepTime.value - this->baseTime) << (7 + 4)) / 125 + 1; // sleep for one count longer ...
			NRF_RTC0->CC[0] = timeout;
			NRF_RTC0->EVENTS_COMPARE[0] = 0;
			nvic::clear(RTC0_IRQn);

			// wait if timeout has not passed yet
			bool notPassed = ((timeout - int32_t(NRF_RTC0->COUNTER)) << 8) > (1 << 8); // ... because we have to treat the next count as "passed"
			if (notPassed) {
				//debug::toggleRed();

				// data synchronization barrier
				__DSB();

				// wait for event (interrupts trigger an event due to SEVONPEND)
				__WFE();
			}
		}

		// call handlers
		Handler *handler;
		while ((handler = this->handlerQueue.pop()) != nullptr) {
			handler->handle();
		}

		// resume coroutines waiting on sleep()
		currentTime = now();
		this->sleepTasks1.doUntil(currentTime);
		this->sleepTasks2.doUntil(currentTime);
	}
	this->exitFlag = false;
}

Loop::Time Loop_RTC0::now() {
	// time resolution 1/1000 s
	uint32_t counter = NRF_RTC0->COUNTER;
	if (NRF_RTC0->EVENTS_OVRFLW) {
		NRF_RTC0->EVENTS_OVRFLW = 0;

		// reload counter in case overflow happened after reading the counter
		counter = NRF_RTC0->COUNTER;

		// advance base time by one interval (1024 seconds)
		this->baseTime += INTERVAL;
	}
	return Time(this->baseTime + ((counter * 125) >> (7 + 4)));
}

Awaitable<CoroutineTimedTask> Loop_RTC0::sleep(Time time) {
	return {this->sleepTasks2, time};
}

} // namespace coco
