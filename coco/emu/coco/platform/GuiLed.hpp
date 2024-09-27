#pragma once

#include "Gui.hpp"


namespace coco {

/**
 * Debug LED on the emulator gui.
 * Usage: gui.draw<Led>(color);
 */
class GuiLed : public Gui::Renderer {
public:
	GuiLed();

	float2 draw(float2 position, int color);

protected:
	GLint colorUniform;
};

} // namespace coco
