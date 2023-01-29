#pragma once

#include <coco/Loop.hpp>
#include "Handler.hpp"


namespace coco {

/**
 * Implementation of the Loop interface using Win32 and io completion ports
 */
class Loop_Win32 : public Loop {
public:

	Loop_Win32();
	~Loop_Win32() override;

	void run() override;
	[[nodiscard]] Awaitable<> yield() override;
	[[nodiscard]] Time virtual now() override;
	[[nodiscard]] virtual Awaitable<Time> sleep(Time time) override;

	// handle events and wait at most the given number of milliseconds for new events
	bool handleEvents(int wait = std::numeric_limits<int>::max() / 2);

	// io completion port
	HANDLE port;

	YieldHandlerList yieldHandlers;
	TimeHandlerList timeHandlers;

protected:

	// coroutines waiting on yield()
	Waitlist<> yieldWaitlist;

	// coroutines waiting on sleep()
	Waitlist<Time> sleepWaitlist;
};

} // namespace coco
