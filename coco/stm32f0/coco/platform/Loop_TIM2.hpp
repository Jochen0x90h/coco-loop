#pragma once

#include <coco/Loop.hpp>
#include <coco/LinkedList.hpp>


namespace coco {

/*
	Implementation of the Loop interface using RTC0

	Reference manual:
		https://www.st.com/resource/en/reference_manual/dm00031936-stm32f0x1stm32f0x2stm32f0x8-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

	Resources:
		TIM2
			CC1
*/
class Loop_TIM2 : public Loop {
public:

	Loop_TIM2() = default;
	~Loop_TIM2() override;

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

	// coroutines waiting on yield()
	TaskList<> yieldTaskList;

	// coroutines waiting on sleep()
	TaskList<Time> sleepTaskList;
};

} // namespace coco
