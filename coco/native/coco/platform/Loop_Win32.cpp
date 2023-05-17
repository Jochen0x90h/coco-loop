#include "Loop_Win32.hpp"
#include <iterator>
#include <iostream>


namespace coco {

Loop_Win32::Loop_Win32() {
	this->port = CreateIoCompletionPort(
		INVALID_HANDLE_VALUE, // FileHandle,
		nullptr, // ExistingCompletionPort,
		NULL, // CompletionKey,
		1 // NumberOfConcurrentThreads
	);
	if (this->port == INVALID_HANDLE_VALUE) {
		auto e = GetLastError();
		std::cout << "CreateIoCompletionPort: " << e << std::endl;
	}
}

Loop_Win32::~Loop_Win32() {
	CloseHandle(this->port);
}

void Loop_Win32::run(const int &condition) {
	int c = condition;
	while (c == condition) {
		handleEvents();
	}
}

Awaitable<> Loop_Win32::yield() {
	return {this->yieldTaskList};
}

Time Loop_Win32::now() {
	return {timeGetTime()};
}

Awaitable<Time> Loop_Win32::sleep(Time time) {
	return {this->sleepTaskList, time};
}

bool Loop_Win32::handleEvents(int wait) {
	// resume coroutines waiting on yield() and activate yield handlers
	{
		this->yieldTaskList.resumeAll();

		auto it = this->yieldHandlers.begin();
		while (it != this->yieldHandlers.end()) {
			// increment iterator beforehand because a yield handler can remove() itself
			auto &handler = *it;
			++it;

			handler.handle();
		}
	}

	// resume coroutines waiting on sleep() and activate time handlers
	// todo: keep sleepWaitlist in sorted order as optimization
	Time currentTime = now();
	this->sleepTaskList.resumeAll([currentTime](Time time) {
		// check if this time has elapsed
		return time <= currentTime;
	});
	auto it = this->timeHandlers.begin();
	while (it != this->timeHandlers.end()) {
		// increment iterator beforehand because a time handler can remove() itself
		auto &handler = *it;
		++it;

		if (handler.time <= currentTime)
			handler.handle();
	}

	// determine timeout
	int timeout = 0;
	if (this->yieldTaskList.empty() && this->yieldHandlers.empty()) {
		// get sleep time
		Time sleepTime = {currentTime.value + wait};
		this->sleepTaskList.visitAll([&sleepTime](Time time) {
			// check if this time is the next to elapse
			if (time < sleepTime)
				sleepTime = time;
		});
		for (auto &handler : this->timeHandlers) {
			if (handler.time < sleepTime)
				sleepTime = handler.time;
			++it;
		}

		int t = (sleepTime - now()).value;
		timeout = t > 0 ? t : 0;
	}

	// wait for io completion
	ULONG entryCount;
	OVERLAPPED_ENTRY entries[16];
	auto result = GetQueuedCompletionStatusEx(
		this->port,
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
		return true;
	} else {
		// timeout
		auto e = GetLastError();
		if (e != WAIT_TIMEOUT)
			std::cout << "GetQueuedCompletionStatusEx: " << e << std::endl;
		return false;
	}

}


// Loop_Win32::YieldHandler

Loop_Win32::YieldHandler::~YieldHandler() {
}


// Loop_Win32::TimeHandler

Loop_Win32::TimeHandler::~TimeHandler() {
}


// Loop_Win32::CompletionHandler

Loop_Win32::CompletionHandler::~CompletionHandler() {
}

} // namespace coco
