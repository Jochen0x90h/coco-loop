#include "Newline_emu.hpp"


namespace coco {

Newline_emu::Newline_emu(Loop_emu &loop) {
	loop.guiHandlers.add(*this);
}

Newline_emu::~Newline_emu() {
}

void Newline_emu::handle(Gui &gui) {
	gui.newline();
}

} // namespace coco
