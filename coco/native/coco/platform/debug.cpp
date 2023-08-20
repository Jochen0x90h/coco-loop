#include <coco/assert.hpp>
#include <iostream>


namespace coco {
namespace debug {

void init() {}

void setRed(bool value = true) {
	std::cout << "Red " << (value ? "on" : "off") << std::endl;
}

void toggleRed() {
	std::cout << "Red toggle" << std::endl;
}

void setGreen(bool value = true) {
	std::cout << "Green " << (value ? "on" : "off") << std::endl;
}

void toggleGreen() {
	std::cout << "Green toggle" << std::endl;
}

void setBlue(bool value = true) {
	std::cout << "Blue " << (value ? "on" : "off") << std::endl;
}

void toggleBlue() {
	std::cout << "Blue toggle" << std::endl;
}

} // namespace debug
} // namespace coco
