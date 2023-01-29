#pragma once

#include <coco/Coroutine.hpp>
#include <coco/Time.hpp>


namespace coco {

class Loop {
public:
	virtual ~Loop();

	/**
	 * Run the event loop
	 */
	virtual void run() = 0;

	/**
	 * Yield control to other coroutines. Can be used to do longer processing in a cooperative way.
	 */
	[[nodiscard]] virtual Awaitable<> yield() = 0;

	/**
	 * Get current time in milliseconds
	 * @return current time
	 */
	[[nodiscard]] Time virtual now() = 0;

	/**
	 * Suspend execution using co_await until a given time. Only up to TIMER_COUNT coroutines can wait simultaneously.
	 * @param time time point
	 */
	[[nodiscard]] virtual Awaitable<Time> sleep(Time time) = 0;

	/**
	 * Suspend execution using co_await for a given duration. Only up to TIMER_COUNT coroutines can wait simultaneously.
	 * @param duration duration
	 */
	[[nodiscard]] Awaitable<Time> sleep(Duration duration) {return sleep(now() + duration);}
};

} // namespace coco
