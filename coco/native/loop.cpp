#include "loop.inc.hpp"


namespace coco {
namespace loop {

void init() {
	initTimer();
}

void run() {
	while (true) {
		handleEvents();
	}
}

} // namespace loop
} // namespace coco
