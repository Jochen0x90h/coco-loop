#pragma once

#include <coco/Loop.hpp>
#include "Handler.hpp"


namespace coco {

/*
 * Implementation of the Loop interface using RTC0
 * https://infocenter.nordicsemi.com/topic/struct_nrf52/struct/nrf52840.html
 *
 * Resources:
 *	NRF_RTC0
 *		CC[0]
 */
class Loop_RTC0 : public Loop {
public:

	Loop_RTC0() = default;
	~Loop_RTC0() override;

	void run() override;
	[[nodiscard]] Awaitable<> yield() override;
	[[nodiscard]] Time virtual now() override;
	[[nodiscard]] virtual Awaitable<Time> sleep(Time time) override;

	HandlerList handlers;

protected:

	// base time for now() because the RTC counter is only 24 bit and runs at 16384Hz
	uint32_t baseTime = 0;

	// coroutines waiting on yield()
	Waitlist<> yieldWaitlist;

	// coroutines waiting on sleep()
	Waitlist<Time> sleepWaitlist;
};

} // namespace coco
