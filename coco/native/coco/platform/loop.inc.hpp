#include <coco/loop.hpp>
#include <coco/platform/Handler.hpp>
#ifdef _WIN32
#define NOMINMAX
#include <WinSock2.h>
#undef interface
#undef INTERFACE
#undef IN
#undef OUT
#else
#include <ctime>
#include <poll.h>
#endif
#include <iostream>


namespace coco {
namespace loop {

// maximum number of sockets we can poll
constexpr int MAX_POLL_COUNT = 16;

// coroutines waiting on yield()
Waitlist<> yieldWaitlist;

// coroutines waiting on sleep()
Waitlist<Time> sleepWaitlist;


Awaitable<> yield() {
	return {loop::yieldWaitlist};
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
	return {loop::sleepWaitlist, time};
}


#ifdef _WIN32
static int poll(struct pollfd *fdArray, int fds, int timeout) {
	if (fds == 0) {
		Sleep(timeout);
		return 0;
	}
	return WSAPoll(fdArray, fds, timeout);
}
#endif


/**
 * Platform dependent function: Handle events
 * @param wait wait for an event or timeout. Set to false when waiting is not wanted, e.g. in a rendering loop when using GLFW
 */
static void handleEvents(bool wait = true) {
	// resume coroutines waiting on yield() and activate yield handlers
	{
		loop::yieldWaitlist.resumeAll();

		auto it = coco::yieldHandlers.begin();
		while (it != coco::yieldHandlers.end()) {
			// increment iterator beforehand because a yield handler can remove() itself
			auto &handler = *it;
			++it;

			handler.activate();
		}
	}

	// resume coroutines waiting on sleep() and activate time handlers
	// todo: keep sleepWaitlist in sorted order as optimization
	Time currentTime = loop::now();
	loop::sleepWaitlist.resumeAll([currentTime](Time time) {
		// check if this time has elapsed
		return time <= currentTime;
	});
	auto it = coco::timeHandlers.begin();
	while (it != coco::timeHandlers.end()) {
		// increment iterator beforehand because a time handler can remove() itself
		auto &handler = *it;
		++it;

		if (handler.time <= currentTime)
			handler.activate();
	}

	// get sleep time
	Time sleepTime = {currentTime.value + Duration::max().value / 2};
	loop::sleepWaitlist.visitAll([&sleepTime](Time time) {
		// check if this time is the next to elapse
		if (time < sleepTime)
			sleepTime = time;
	});
	for (auto &handler : coco::timeHandlers) {
		if (handler.time < sleepTime)
			sleepTime = handler.time;
		++it;
	}

	// fill poll infos
	struct pollfd infos[MAX_POLL_COUNT];
	int count = 0;
	for (auto &fileDescriptor : coco::socketHandlers) {
		infos[count++] = {fileDescriptor.socket, fileDescriptor.events, 0};
	}
	assert(count <= MAX_POLL_COUNT);

	// determine poll timeout
	int32_t timeout = 0;
	if (wait && loop::yieldWaitlist.empty() && coco::yieldHandlers.empty()) {
		auto t = (sleepTime - loop::now()).value;
		timeout = t > 0 ? t : 0;
	}

	// poll
	//std::cout << "poll count " << count << " timeout " << timeout << std::endl;
	int r = poll(infos, count, timeout);

	// activate file descriptors
	if (r > 0) {
		int i = 0;
		auto it = coco::socketHandlers.begin();
		while (it != coco::socketHandlers.end()) {
			// increment iterator beforehand because a socket handler can remove() itself
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
