#include "Loop_Win32.hpp"
#include <iterator>
#include <iostream>


namespace coco {

Loop_Win32::Loop_Win32() {
	// get frequency of QueryPerformanceCounter()
	// https://learn.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps
	// http://www.geisswerks.com/ryan/FAQS/timing.html
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	frequency_ = frequency.QuadPart / 1000;

	// create io completion port
	port = CreateIoCompletionPort(
		INVALID_HANDLE_VALUE, // FileHandle,
		nullptr, // ExistingCompletionPort,
		NULL, // CompletionKey,
		1 // NumberOfConcurrentThreads
	);
	if (port == INVALID_HANDLE_VALUE) {
		auto e = GetLastError();
		std::cout << "CreateIoCompletionPort: " << e << std::endl;
	}
}

Loop_Win32::~Loop_Win32() {
	CloseHandle(port);
}

void Loop_Win32::run() {
	while (!exitFlag_) {
		handleEvents();
	}
	exitFlag_ = false;
}

Loop::Time Loop_Win32::now() {
	// todo: handle overflow
	LARGE_INTEGER time;
	QueryPerformanceCounter(&time);
	return Time(time.QuadPart / frequency_);
}

Awaitable<CoroutineTimedTask> Loop_Win32::sleep(Time time) {
	return {sleepTasks2_, time};
}

bool Loop_Win32::handleEvents(int wait) {
	// determine timeout, only sleep if there are no coroutines waiting on yield()
	int timeout = 0;
	{
		Time currentTime = now();
		Time sleepTime = sleepTasks2_.getFirstTime(sleepTasks1_.getFirstTime(currentTime + wait * 1ms));
		int t = (sleepTime - currentTime).value;
		timeout = t > 0 ? t : 0;
	}

	// wait for io completion
	ULONG entryCount;
	OVERLAPPED_ENTRY entries[16];
	bool result = GetQueuedCompletionStatusEx(
		port,
		entries,
		std::size(entries),
		&entryCount,
		timeout,
		false);
	if (result) {
		// one or more operations completed: call handler
		for (int i = 0; i < entryCount; ++i) {
			auto &entry = entries[i];
			auto handler = (CompletionHandler *)(entry.lpCompletionKey);
			handler->handle(entry.lpOverlapped);
		}
	} else {
		// timeout
		auto e = GetLastError();
		if (e != WAIT_TIMEOUT)
			std::cout << "GetQueuedCompletionStatusEx: " << e << std::endl;
	}

	// resume coroutines waiting on yield() and activate yield handlers
	//yieldTasks1.doAll();
	//yieldTasks2.doAll();

	// resume coroutines waiting on sleep() and activate time handlers
	{
		Time currentTime = now();
		sleepTasks1_.doUntil(currentTime);
		sleepTasks2_.doUntil(currentTime);
	}

	return result;
}


// Loop_Win32::CompletionHandler

Loop_Win32::CompletionHandler::~CompletionHandler() {
}

} // namespace coco
