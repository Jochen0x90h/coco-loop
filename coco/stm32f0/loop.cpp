#include "../loop.hpp"
#include <coco/platform/Handler.hpp>
#include <coco/platform/platform.hpp>


/*
	Documentation:
		https://www.st.com/resource/en/reference_manual/dm00031936-stm32f0x1stm32f0x2stm32f0x8-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

	Dependencies:

	Resources:
		TIM2
			CCR1
*/
namespace coco {
namespace loop {

constexpr int CLOCK = 8000000;

// next timeout of a timer in the list
Time next;

// waiting coroutines
Waitlist<Time> waitlist;


void init() {
	// disabled interrupts trigger an event and wake up the processor from WFE
	// see chapter 5.3.3 in reference manual
	SCB->SCR = SCB->SCR | SCB_SCR_SEVONPEND_Msk;

	// initialize TIM2
	RCC->APB1ENR = RCC->APB1ENR | RCC_APB1ENR_TIM2EN;
	loop::next.value = Duration::max().value;
	TIM2->CCR1 = loop::next.value;
	TIM2->PSC = (CLOCK + 1000 / 2) / 1000 - 1; // prescaler for 1ms timer resolution
	TIM2->EGR = TIM_EGR_UG; // update generation so that prescaler takes effect
	TIM2->DIER = TIM_DIER_CC1IE; // interrupt enable for CC1
	TIM2->CR1 = TIM_CR1_CEN; // enable, count up
}

void run() {
	while (true) {
		// check if a sleep time has elapsed
		if (TIM2->SR & TIM_SR_CC1IF) {
			do {
				// clear pending interrupt flags at peripheral and NVIC
				TIM2->SR = ~TIM_SR_CC1IF;
				clearInterrupt(TIM2_IRQn);

				auto time = loop::next;
				loop::next += Duration::max();

				// resume all coroutines that where timeout occurred
				loop::waitlist.resumeAll([time](Time timeout) {
					if (timeout == time)
						return true;

					// check if this time is the next to elapse
					if (timeout < loop::next)
						loop::next = timeout;
					return false;
				});
				TIM2->CCR1 = loop::next.value;

				// repeat until next timeout is in the future
			} while (Time(TIM2->CNT) >= loop::next);
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
	return {TIM2->CNT};
}

Awaitable<Time> sleep(Time time) {
	// check if this time is the next to elapse
	if (time < loop::next) {
		loop::next = time;
		TIM2->CCR1 = time.value;
	}

	// check if timeout already elapsed
	if (Time(TIM2->CNT) >= time)
		TIM2->EGR = TIM_EGR_CC1G; // trigger compare event

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
