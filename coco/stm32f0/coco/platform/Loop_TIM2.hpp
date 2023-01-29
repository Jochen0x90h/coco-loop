#pragma once

#include <coco/Loop.hpp>
#include "Handler.hpp"


namespace coco {

/*
 * Implementation of the Loop interface using RTC0
 * https://www.st.com/resource/en/reference_manual/dm00031936-stm32f0x1stm32f0x2stm32f0x8-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
 *
 * Resources:
 *	TIM2
 *		CC1
 */
class Loop_TIM2 : public Loop {
public:

	Loop_TIM2() = default;
	~Loop_TIM2() override;

	void run() override;
	[[nodiscard]] Awaitable<> yield() override;
	[[nodiscard]] Time virtual now() override;
	[[nodiscard]] virtual Awaitable<Time> sleep(Time time) override;

	HandlerList handlers;

protected:

	// coroutines waiting on yield()
	Waitlist<> yieldWaitlist;

	// coroutines waiting on sleep()
	Waitlist<Time> sleepWaitlist;
};

} // namespace coco
