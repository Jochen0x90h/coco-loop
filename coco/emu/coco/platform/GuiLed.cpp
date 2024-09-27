#include "GuiLed.hpp"


namespace coco {

GuiLed::GuiLed()
	: Renderer("#version 330\n"
		"uniform vec4 color;\n"
		"in vec2 xy;\n"
		"out vec4 pixel;\n"
		"void main() {\n"
			"vec2 a = xy - vec2(0.5, 0.5f);\n"
			"float length = sqrt(a.x * a.x + a.y * a.y);\n"
			"float s = clamp((length - 0.3) * 10.0, 0.0, 1.0);\n"
			"vec4 background = vec4(0, 0, 0, 1);\n"
			"pixel = (1.0 - s) * color + s * background;\n"
		"}\n")
{
	this->colorUniform = getUniformLocation("color");
}

float2 GuiLed::draw(float2 position, int color) {
	const float2 size = {0.025f, 0.025f};

	setState(position, size);
	float r = float(color & 0xff) / 255.0f;
	float g = float((color >> 8) & 0xff) / 255.0f;
	float b = float((color >> 16) & 0xff) / 255.0f;
	glUniform4f(this->colorUniform, r, g, b, 1.0f);
	drawAndResetState();

	return size;
}

} // namespace coco
