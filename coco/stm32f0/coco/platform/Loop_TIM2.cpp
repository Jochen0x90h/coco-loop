#include "Loop_TIM2.hpp"
#include <coco/platform/platform.hpp>


namespace coco {

Loop_TIM2::~Loop_TIM2() {
}

void Loop_TIM2::run() {
	while (true) {
		// resume coroutines waiting on yield()
		this->yieldWaitlist.resumeAll();

		// resume coroutines waiting on sleep()
		// todo: keep sleepWaitlist in sorted order as optimization
		Time currentTime = now();
		this->sleepWaitlist.resumeAll([currentTime](Time time) {
			// check if this time has elapsed
			return time <= currentTime;
		});

		// get sleep time
		Time sleepTime = {currentTime.value + Duration::max().value / 2};
		this->sleepWaitlist.visitAll([&sleepTime](Time time) {
			// check if this time is the next to elapse
			if (time < sleepTime)
				sleepTime = time;
		});

		// only sleep if there are no coroutines waiting on yield()
		if (this->yieldWaitlist.empty()) {
			// set new timeout and clear pending interrupt flags at peripheral and NVIC
			TIM2->CCR1 = sleepTime.value;
			TIM2->SR = ~TIM_SR_CC1IF;
			clearInterrupt(TIM2_IRQn);
		
			// wait for event
			// see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0321a/BIHICBGB.html
			if (now() < sleepTime) {
				//debug::toggleGreen();

				// data synchronization barrier
				__DSB();
			
				// wait for event (interrupts trigger an event due to SEVONPEND)
				__WFE();
			}
		}

		// call all handlers
		auto it = this->handlers.begin();
		while (it != this->handlers.end()) {
			// increment iterator beforehand because a handler can remove() itself
			auto current = it;
			++it;
			current->handle();
		}
	}
}

Awaitable<> Loop_TIM2::yield() {
	return {this->yieldWaitlist};
}

Time Loop_TIM2::now() {
	return {TIM2->CNT};
}

Awaitable<Time> Loop_TIM2::sleep(Time time) {
	// todo: insert into waitlist in sorted order
	return {this->sleepWaitlist, time};
}
/*
void sleepBlocking(int us) {
	auto cycles = us * 8;
	for (int i = 0; i < cycles; ++i) {
		__NOP();
	}
}*/

} // namespace coco
