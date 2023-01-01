#include "../loop.hpp"
#include <coco/platform/Handler.hpp>
#ifdef _WIN32
#define NOMINMAX
#include <WinSock2.h>
#undef interface
#undef INTERFACE
#undef IN
#undef OUT
#define poll WSAPoll
#else
#include <ctime>
#include <poll.h>
#endif


namespace coco {
namespace loop {

// maximum number of sockets we can poll
constexpr int MAX_POLL_COUNT = 16;


// Timer

class Timer : public TimeHandler {
public:
	void activate() override {
		auto time = this->time;
		this->time += Duration::max();

		// resume all coroutines that where timeout occurred
		this->waitlist.resumeAll([this, time](Time timeout) {
			if (timeout == time)
				return true;

			// check if this time is the next to elapse
			if (timeout < this->time)
				this->time = timeout;
			return false;
		});
	}

	// waiting coroutines
	Waitlist<Time> waitlist;
};
Timer timer;


/**
 * Platform dependent function: Initialize timer
 */
static void initTimer() {
	// check if not yet initialized
	assert(!loop::timer.isInList());

	loop::timer.time = loop::now() + Duration::max();
	coco::timeHandlers.add(loop::timer);
}

Time now() {
#ifdef _WIN32
	return {timeGetTime()};
#else
	timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	return {uint32_t(time.tv_sec * 1000 + time.tv_nsec / 1000000)};
#endif
}

Awaitable<Time> sleep(Time time) {
	// check if initTimer() was called
    assert(loop::timer.isInList());

    // check if this time is the next to elapse
	if (time < loop::timer.time)
		loop::timer.time = time;

	return {loop::timer.waitlist, time};
}


/**
 * Platform dependent function: Handle events
 * @param wait wait for an event or timeout. Set to false when using a rendering loop, e.g. when using GLFW
 */
static void handleEvents(bool wait = true) {
	// check if initTimer() was called
    assert(loop::timer.isInList());	
	
	// activate timeouts
	Time time;
	bool activated;
	do {
		time = loop::now();
		activated = false;
		auto it = coco::timeHandlers.begin();
		while (it != coco::timeHandlers.end()) {
			// increment iterator beforehand because a timer can remove() itself
			auto current = it;
			++it;

			// check if timer needs to be activated
			if (current->time <= time) {
				current->activate();
				activated = true;
			}
		}
	} while (activated);

	// get next timeout
	auto next = time + Duration::max();
	for (auto &timeout : coco::timeHandlers) {
		if (timeout.time < next)
			next = timeout.time;
	}

	// fill poll infos
	struct pollfd infos[MAX_POLL_COUNT];
	int count = 0;
	for (auto &fileDescriptor : coco::socketHandlers) {
		infos[count++] = {fileDescriptor.socket, fileDescriptor.events, 0};
	}
	assert(count <= MAX_POLL_COUNT);

	// poll
	auto timeout = (next - time).value;
	//Terminal::out << "timeout " << dec(timeout) << '\n';
	int r = poll(infos, count, (timeout > 0 && wait) ? timeout : 0);

	// activate file descriptors
	if (r > 0) {
		int i = 0;
		auto it = coco::socketHandlers.begin();
		while (it != coco::socketHandlers.end()) {
			// increment iterator beforehand because a file descriptor can remove() itself
			auto current = it;
			++it;

			// check if file descriptor needs to be activated
			auto events = infos[i].revents;
			if (events != 0)
				current->activate(events);

			// "garbage collect" file descriptors that are not interested in events anymore, also after close() was called
			if (current->events == 0)
				current->remove();

			++i;
		}
	}
}

} // namespace loop
} // namespace coco
