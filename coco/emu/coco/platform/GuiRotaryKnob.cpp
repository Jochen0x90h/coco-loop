#include "GuiRotaryKnob.hpp"
#include <cmath>
#include <numbers>


namespace coco {

using namespace std::numbers;

constexpr float PERIOD = 65536.0f / (2.0f * pi_v<float>);

GuiRotaryKnob::~GuiRotaryKnob() {
}

GuiRotaryKnob::Result GuiRotaryKnob::update(Gui &gui) {
	// draw and resize the widget
	float angle = float(this->value & 0xffff) / (PERIOD * this->increments);
	int offset = int(this->button) * 4;
	const float outerColor[] = {0.7f, 0.7f, 0.7f, 1.0f};
	const float innerColor[] = {0, 0, 0, 1,  1, 1, 1, 1};
	resize(gui.draw<Wheel>(this->radius, outerColor, innerColor + offset, this->increments, angle));

	// calculate the result
	Result result;

	// delta
	auto value = this->value & 0xffff0000;
	int delta = int32_t(value - this->lastValue) >> 16;
	this->lastValue = value;
	if (delta != 0) {
		// clockwise is positive e.g. for a volume knob
		result.delta = -delta;
	}

	// button
	if (this->haveButton) {
		bool toggle = this->button != this->lastButton;
		this->lastButton = this->button;
		if (toggle)
			result.button = this->button;
	}

	return result;
}

void GuiRotaryKnob::touch(bool first, float x, float y) {
	// current vector on widget where (0, 0) is at the center
	float ax = x - 0.5f;
	float ay = y - 0.5f;
	bool inner = this->haveButton && std::sqrt(ax * ax + ay * ay) < this->radius;
	if (first) {
		// check if button (inner circle) was hit
		this->button = inner;
	} else if (!inner) {
		// last vector on widget
		float bx = this->x - 0.5f;
		float by = this->y - 0.5f;

		// calc angle (in rad) between vectors (approximation, is actually sin(angle))
		float d = (ay * bx - ax * by) / (std::sqrt(ax * ax + ay * ay) * std::sqrt(bx * bx + by * by));
		this->value = this->value - int(d * PERIOD * this->increments);
	}

	// store for next call to touch()
	this->x = x;
	this->y = y;
}

void GuiRotaryKnob::release() {
	this->button = false;
}


// Wheel

GuiRotaryKnob::Wheel::Wheel()
	: Gui::Renderer("#version 330\n"
		"uniform float innerRadius;\n"
		"uniform float increments;\n"
		"uniform float angle;\n"
		"uniform vec4 outerColor;\n"
		"uniform vec4 innerColor;\n"
		"in vec2 xy;\n"
		"out vec4 pixel;\n"
		"void main() {\n"
		"vec2 p = xy - vec2(0.5, 0.5f);\n"
		"float a = atan(p.y, p.x) + angle;\n"
		"float radius = sqrt(p.x * p.x + p.y * p.y);\n"
		"float outerRadius = cos(a * increments) * 0.02 + 0.48;\n"
		//"float innerRadius = 0.4;\n"
		"float delta = 0.01;\n"
		"float outerMix = smoothstep(outerRadius - delta, outerRadius, radius);\n"
		"float innerMix = smoothstep(innerRadius - delta, innerRadius, radius);\n"
		"vec4 color = (1.0 - innerMix) * innerColor + innerMix * outerColor;\n"
		"pixel = (1.0 - outerMix) * color + outerMix * vec4(0, 0, 0, 1);\n"
		"}\n")
{
	this->innerRadiusUniform = getUniformLocation("innerRadius");
	this->incrementsUniform = getUniformLocation("increments");
	this->angleUniform = getUniformLocation("angle");
	this->outerColorUniform = getUniformLocation("outerColor");
	this->innerColorUniform = getUniformLocation("innerColor");
}

float2 GuiRotaryKnob::Wheel::draw(float2 position, float radius, const float *outerColor, const float *innerColor,
	int increments, float angle)
{
	const float2 size{0.2f, 0.2f};

	setState(position, size);
	glUniform1f(this->innerRadiusUniform, radius);
	glUniform4fv(this->outerColorUniform, 1, outerColor);
	glUniform4fv(this->innerColorUniform, 1, innerColor);
	glUniform1f(this->incrementsUniform, float(increments));
	glUniform1f(this->angleUniform, angle);
	drawAndResetState();

	return size;
}

} // namespace coco
