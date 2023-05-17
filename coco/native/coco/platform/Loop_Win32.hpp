#pragma once

#include <coco/Loop.hpp>
#include <coco/LinkedList.hpp>
#include <coco/Time.hpp>

#define NOMINMAX
#include "Windows.h"
#undef interface
#undef INTERFACE
#undef IN
#undef OUT
#undef READ_ATTRIBUTES
#undef ERROR


namespace coco {

/**
	Implementation of the Loop interface using Win32 and io completion ports
*/
class Loop_Win32 : public Loop {
public:

	Loop_Win32();
	~Loop_Win32() override;

	void run(const int &condition) override;
	using Loop::run;
	[[nodiscard]] Awaitable<> yield() override;
	[[nodiscard]] Time now() override;
	[[nodiscard]] Awaitable<Time> sleep(Time time) override;


	/**
		Yield handler used to execute something from the event loop
	*/
	class YieldHandler : public LinkedListNode {
	public:
		virtual ~YieldHandler();
		virtual void handle() = 0;
	};
	LinkedList<YieldHandler> yieldHandlers;

	/**
		Time handler used for delayed execution
	*/
	class TimeHandler : public LinkedListNode {
	public:
		virtual ~TimeHandler();
		virtual void handle() = 0;

		Time time;
	};
	LinkedList<TimeHandler> timeHandlers;

	/**
		IO Completion handler
	*/
	class CompletionHandler {
	public:
		virtual ~CompletionHandler();
		virtual void handle(OVERLAPPED *overlapped) = 0;
	};

	// handle events and wait at most the given number of milliseconds for new events
	bool handleEvents(int wait = std::numeric_limits<int>::max() / 2);

	// io completion port
	HANDLE port;

protected:

	// coroutines waiting on yield()
	TaskList<> yieldTaskList;

	// coroutines waiting on sleep()
	TaskList<Time> sleepTaskList;
};

} // namespace coco
