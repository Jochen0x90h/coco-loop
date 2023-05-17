#pragma once

#include <coco/Loop.hpp>
#include <coco/LinkedList.hpp>


namespace coco {

/*
	Implementation of the Loop interface using RTC0

	Reference manual:
		https://infocenter.nordicsemi.com/topic/struct_nrf52/struct/nrf52840.html

	Resources:
		NRF_RTC0
			CC[0]
*/
class Loop_RTC0 : public Loop {
public:

	Loop_RTC0() = default;
	~Loop_RTC0() override;

	void run(const int &condition) override;
	using Loop::run;
	[[nodiscard]] Awaitable<> yield() override;
	[[nodiscard]] Time now() override;
	[[nodiscard]] Awaitable<Time> sleep(Time time) override;


	/**
		Event handler that handles activity of the peripherals
	*/
	class Handler : public LinkedListNode {
	public:
		virtual ~Handler();
		virtual void handle() = 0;
	};
	LinkedList<Handler> handlers;

protected:

	// base time for now() because the RTC counter is only 24 bit and runs at 16384Hz
	uint32_t baseTime = 0;

	// coroutines waiting on yield()
	TaskList<> yieldTaskList;

	// coroutines waiting on sleep()
	TaskList<Time> sleepTaskList;
};

} // namespace coco
