#include <coco/assert.hpp>


namespace coco {
namespace debug {

bool red = false;
bool green = false;
bool blue = false;

void init() {}

void setRed(bool value) {
	debug::red = value;
}

void toggleRed() {
	debug::red = !debug::red;
}

void setGreen(bool value) {
	debug::green = value;
}

void toggleGreen() {
	debug::green = !debug::green;
}

void setBlue(bool value) {
	debug::blue = value;
}

void toggleBlue() {
	debug::blue = !debug::blue;
}

} // namespace debug
} // namespace coco
