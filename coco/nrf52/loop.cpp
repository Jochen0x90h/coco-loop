#include "../loop.hpp"
#include <coco/platform/Handler.hpp>
#include <coco/platform/platform.hpp>


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

uint32_t baseTime = 0;

// next timeout of a timer in the list
Time next;

// waiting coroutines
Waitlist<Time> waitlist;


void init() {
	// disabled interrupts trigger an event and wake up the processor from WFE
	SCB->SCR = SCB->SCR | SCB_SCR_SEVONPEND_Msk;

	// initialize RTC0
	loop::next.value = INTERVAL - 1;
	NRF_RTC0->CC[0] = next.value << 4;
	NRF_RTC0->EVTENSET = N(RTC_EVTENSET_OVRFLW, Set);
	NRF_RTC0->INTENSET = N(RTC_INTENSET_COMPARE0, Set);
	NRF_RTC0->PRESCALER = 1; // 16384Hz
	NRF_RTC0->TASKS_START = TRIGGER;
}

void run() {
	while (true) {
		// check if a sleep time has elapsed
		if (NRF_RTC0->EVENTS_COMPARE[0]) {
			do {
				// clear pending interrupt flags at peripheral and NVIC
				NRF_RTC0->EVENTS_COMPARE[0] = 0;
				clearInterrupt(RTC0_IRQn);

				auto time = loop::next;
				loop::next.value += INTERVAL - 1;

				// resume all coroutines that where timeout occurred
				loop::waitlist.resumeAll([time](Time timeout) {
					if (timeout == time)
						return true;

					// check if this time is the next to elapse
					if (timeout < loop::next)
						loop::next = timeout;
					return false;
				});
				NRF_RTC0->CC[0] = ((loop::next.value - loop::baseTime) << (7 + 4)) / 125;//Timer::next.value << 4;

				// repeat until next timeout is in the future
			} while (now() >= loop::next);
		}
		
		// call all handlers
		auto it = coco::handlers.begin();
		while (it != coco::handlers.end()) {
			// increment iterator beforehand because a handler can remove() itself
			auto current = it;
			++it;
			current->handle();
		}

		// wait for event
		// see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0321a/BIHICBGB.html
		{
			// data synchronization barrier
			__DSB();
		
			// wait for event (interrupts trigger an event due to SEVONPEND)
			__WFE();
		}
	}
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
	// check if this time is the next to elapse
	if (time < loop::next) {
		loop::next = time;
		NRF_RTC0->CC[0] = ((time.value - loop::baseTime) << (7 + 4)) / 125;
	}

	// check if timeout already elapsed
	if (now() >= time)
		NRF_RTC0->EVENTS_COMPARE[0] = GENERATED;

	return {loop::waitlist, time};
}

void sleepBlocking(int us) {
	auto cycles = us * 8;
	for (int i = 0; i < cycles; ++i) {
		__NOP();
	}
}

} // namespace loop
} // namespace coco
