#pragma once

#include <coco/Coroutine.hpp>
#include <coco/Time.hpp>


namespace coco {

class Loop {
public:
	virtual ~Loop();

	/**
		Run the event loop until exit() gets called
	*/
	void run() {
		++this->recursion;
		run(this->recursion);
	}

	/**
		Run the event loop until a condition variable changes. Use for example in startup code when reading from files
		or SPI to avoid coroutines. Use with care as other asynchronous events are still handled.
		@param condition condition variable
	*/
	virtual void run(const int &condition) = 0;
	void run(int &condition) {run(const_cast<const int &>(condition));}

	/**
		Run the event loop until a condition variable changes that is an enum with underlying type of int
		@param condition condition variable
	*/
	template <typename E>
	void run(const E &condition) {run(reinterpret_cast<const std::underlying_type_t<E> &>(condition));}
	template <typename E>
	void run(E &condition) {run(reinterpret_cast<const std::underlying_type_t<E> &>(condition));}

	/**
		Do not accept r-value references for run(condition)
	*/
	template <typename T>
	void run(T &&condition) = delete;

	/**
		Exit the event loop on "normal" operating systems
	*/
	void exit() {--this->recursion;}

	/**
		Yield control to other coroutines. Can be used to do longer processing in a cooperative way.
	*/
	[[nodiscard]] virtual Awaitable<> yield() = 0;

	/**
		Get current time in milliseconds
		@return current time
	*/
	[[nodiscard]] Time virtual now() = 0;

	/**
		Suspend execution using co_await until a given time. Only up to TIMER_COUNT coroutines can wait simultaneously.
		@param time time point
	*/
	[[nodiscard]] virtual Awaitable<Time> sleep(Time time) = 0;

	/**
		Suspend execution using co_await for a given duration. Only up to TIMER_COUNT coroutines can wait simultaneously.
		@param duration duration
	*/
	[[nodiscard]] Awaitable<Time> sleep(Duration duration) {return sleep(now() + duration);}


	int recursion = 0;
};

} // namespace coco
