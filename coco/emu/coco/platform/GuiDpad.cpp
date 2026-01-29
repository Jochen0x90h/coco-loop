#include "GuiDpad.hpp"
#include <cmath>
#include <numbers>
#include <ranges>


namespace coco {

using namespace std::numbers;
using std::max;

namespace {

constexpr float PERIOD = 65536.0f / (2.0f * pi_v<float>);
constexpr float2 SIZE = {0.2f, 0.2f};

float length(float2 x) {
    return std::sqrt(x.x * x.x + x.y * x.y);
}

} // namespace


GuiDpad::~GuiDpad() {
}

GuiDpad::Result GuiDpad::update(Gui &gui) {
    // draw and resize the widget
    float4 colors[5];
    for (int i = 0; i < 5; ++i) {
        colors[i] = this->buttons[i] ? float4(1, 1, 1, 1) : float4(0.7f, 0.7f, 0.7f, 1.0f);
        if (!this->haveCenterButton && i == 4)
            colors[4] = float4(0, 0, 0, 1);
    }
    resize(gui.draw<Dpad>(colors));

    // calculate the result
    Result result;
    for (int i = 0; i < 5; ++i) {
        bool button = this->buttons[i];
        bool toggle = button != this->lastButtons[i];
        this->lastButtons[i] = button;
        if (toggle)
            result.buttons[i] = button;
    }

    return result;
}

void GuiDpad::touch(bool first, float x, float y) {
    // current vector on widget where (0, 0) is at the center
    float2 p = {x - 0.5f, y - 0.5f};

    if (first) {
        // check which button was hit
        if (max(abs(p.x), abs(p.y)) > 0.25f && abs(p.x) + abs(p.y) <= 0.5f) {
            int index = (p.x < p.y ? 0 : 1) + (p.x < -p.y ? 0 : 2);
            this->buttons[index] = true;
        }
        if (this->haveCenterButton && length(p) <= 0.2f) {
            this->buttons[4] = true;
        }
    }
}

void GuiDpad::release() {
    std::ranges::fill(this->buttons, false);
}


// Dpad

GuiDpad::Dpad::Dpad()
    : Gui::Renderer("#version 330\n"
        "uniform vec4 colors[5];\n"
        "in vec2 xy;\n"
        "out vec4 pixel;\n"
        "void main() {\n"
        "vec2 p = xy - vec2(0.5, 0.5f);\n"
        "float delta = 0.01;\n"

        "float innerMix = smoothstep(0.25, 0.25 + delta, max(abs(p.x), abs(p.y)));\n"
        "float outerMix = smoothstep(0.5 - delta, 0.5, abs(p.x) + abs(p.y));\n"
        "int index = (p.x < p.y ? 0 : 1) + (p.x < -p.y ? 0 : 2);\n"

        "float centerMix = smoothstep(0.2 - delta, 0.2, sqrt(p.x * p.x + p.y * p.y));\n"

        "pixel = innerMix * (1 - outerMix) * colors[index] + (1 - centerMix) * colors[4];\n"
        "}\n")
{
    this->colorsUniform = getUniformLocation("colors");
}

float2 GuiDpad::Dpad::draw(float2 position, float4 *colors)
{
    setState(position, SIZE);
    glUniform4fv(this->colorsUniform, 5, colors->data());
    drawAndResetState();

    return SIZE;
}

} // namespace coco
