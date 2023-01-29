#pragma once

#include <coco/LinkedList.hpp>


namespace coco {

/**
 * Event handler that handles activity of the peripherals
 */
class Handler : public LinkedListNode<Handler> {
public:
	virtual ~Handler();
	virtual void handle() = 0;
};
using HandlerList = LinkedList<Handler>;

} // namespace coco
