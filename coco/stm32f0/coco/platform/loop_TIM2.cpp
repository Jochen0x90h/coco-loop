#include <coco/loop.hpp>
#include <coco/platform/Handler.hpp>
#include <coco/platform/platform.hpp>
//#include <coco/debug.hpp>


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
		Time sleepTime = {currentTime.value + Duration::max().value / 2};
		loop::sleepWaitlist.visitAll([&sleepTime](Time time) {
			// check if this time is the next to elapse
			if (time < sleepTime)
				sleepTime = time;
		});

		// only sleep if there are no coroutines waiting on yield()
		if (loop::yieldWaitlist.empty()) {
			// set new timeout and clear pending interrupt flags at peripheral and NVIC
			TIM2->CCR1 = sleepTime.value;
			TIM2->SR = ~TIM_SR_CC1IF;
			clearInterrupt(TIM2_IRQn);
		
			// wait for event
			// see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0321a/BIHICBGB.html
			if (loop::now() < sleepTime) {
				//debug::toggleGreen();

				// data synchronization barrier
				__DSB();
			
				// wait for event (interrupts trigger an event due to SEVONPEND)
				__WFE();
			}
		}

		// call all handlers
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
	return {TIM2->CNT};
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
