#pragma once

#include <coco/Loop.hpp>
#include <coco/Callback.hpp>
#include <coco/Frequency.hpp>
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
	enum class Mode {
		// wait for events using WFE instruction
		// Note: Causes connection loss of the debugger on STM32G4
		WAIT,

		// continuously poll for events
		POLL,

		// poll for events with with watchdog of 0.5s
		WATCHDOG
	};

	/**
		Internal constructor
		@param prescaler prescaler for 1ms timer resolution from ABP1 clock
		@param mode loop mode
	*/
	Loop_TIM2(int prescaler, Mode mode);

	/**
		Constructor
		@param clock APB1 timer clock frequency (APB1_TIMER_CLOCK), maximum is 65MHz
		@param mode loop mode
	*/
	Loop_TIM2(Frequency clock, Mode mode = Mode::WAIT) : Loop_TIM2(clock / 1kHz - 1, mode) {}
	~Loop_TIM2() override;

	void run(const int &condition) override;
	using Loop::run;
	[[nodiscard]] Time now() override;
	[[nodiscard]] Awaitable<CoroutineTimedTask> sleep(Time time) override;


	void invoke(TimedTask<Callback> &task, Time time) {
		task.cancelAndSet(time);
		this->sleepTasks1.add(task);
	}

	void invoke(TimedTask<Callback> &task, Duration duration) {
		task.cancelAndSet(now() + duration);
		this->sleepTasks1.add(task);
	}

	void invoke(TimedTask<Callback> &task) {
		task.cancelAndSet(now());
		this->sleepTasks1.add(task);
	}


	/**
		Event handler that handles activity of the peripherals
	*/
	class Handler : public LinkedListNode {
	public:
		virtual ~Handler() {}
		virtual void handle() = 0;
	};
	LinkedList<Handler> handlers;

protected:
	Mode mode;

	// sleep tasks
	TimedTaskList<Callback> sleepTasks1;
	CoroutineTimedTaskList sleepTasks2;
};

} // namespace coco
