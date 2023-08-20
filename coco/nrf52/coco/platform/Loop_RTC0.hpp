#pragma once

#include <coco/Loop.hpp>
#include <coco/Callback.hpp>
#include <coco/LinkedList.hpp>


namespace coco {

/*
	Implementation of the Loop interface using RTC0

	Reference manual:
		https://infocenter.nordicsemi.com/topic/ps_nrf52840/rtc.html?cp=5_0_0_5_21

	Resources:
		NRF_RTC0
			CC[0]
*/
class Loop_RTC0 : public Loop {
public:
	enum class Mode {
		// wait for events using WFE instruction
		WAIT,

		// continuously poll for events
		POLL
	};

	/**
		Constructor
		@param wait wait for events
	*/
	Loop_RTC0(Mode mode = Mode::WAIT);
	~Loop_RTC0() override;

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

	// base time for now() because the RTC counter is only 24 bit and runs at 16384Hz
	uint32_t baseTime = 0;

	// sleep tasks
	TimedTaskList<Callback> sleepTasks1;
	CoroutineTimedTaskList sleepTasks2;
};

} // namespace coco
