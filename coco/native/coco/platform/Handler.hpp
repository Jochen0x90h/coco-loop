#pragma once

#include <coco/LinkedList.hpp>
#include <coco/Time.hpp>

#define NOMINMAX
#include "Windows.h"
#undef interface
#undef INTERFACE
#undef IN
#undef OUT


namespace coco {

/**
 * Yield handler emulated transfers
 */
class YieldHandler : public LinkedListNode<YieldHandler> {
public:
	virtual ~YieldHandler();
	virtual void handle() = 0;
};
using YieldHandlerList = LinkedList<YieldHandler>;


/**
 * Time handler emulated transfers with delay and Clock
 */
class TimeHandler : public LinkedListNode<TimeHandler> {
public:
	virtual ~TimeHandler();
	virtual void handle() = 0;

	Time time;
};
using TimeHandlerList = LinkedList<TimeHandler>;


/**
 * IO Completion handler
 */
class CompletionHandler {
public:
	virtual ~CompletionHandler();

	virtual void handle(OVERLAPPED *overlapped) = 0;

};

} // namespace coco
