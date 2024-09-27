#pragma once

#include "Gui.hpp"


namespace coco {

/**
 * Rotary button (incremental encoder knob with push button) on the emulator gui.
 * Usage: gui.widget<GuiRotaryButton>(id, haveButton);
 */
class GuiRotaryKnob : public Gui::Widget {
public:
	struct Result {
		std::optional<int> delta;
		std::optional<bool> button;
	};

	/**
		Constructor
	*/
	GuiRotaryKnob(int increments, float radius, bool haveButton)
		: increments(increments), radius(radius), haveButton(haveButton), value(0), lastValue(0) {}
	~GuiRotaryKnob() override;

	Result update(Gui &gui);

	void touch(bool first, float x, float y) override;
	void release() override;

protected:
	// renderer for a wheel with button
	class Wheel : public Gui::Renderer {
	public:
		Wheel();

		float2 draw(float2 position, float radius, const float *outerColor, const float *innerColor,
			int increments, float angle);

	protected:
		GLint innerRadiusUniform;
		GLint outerColorUniform;
		GLint innerColorUniform;
		GLint incrementsUniform;
		GLint angleUniform;
	};

	int increments;
	float radius;
	bool haveButton;

	// current (mechanical) increment count in 16:16 format
	uint32_t value;
	uint32_t lastValue;

	// current button state
	bool button = false;
	bool lastButton = false;

	// last mouse position
	float x = 0;
	float y = 0;
};

} // namespace coco
