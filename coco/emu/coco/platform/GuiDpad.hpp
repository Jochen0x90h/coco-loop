#pragma once

#include "Gui.hpp"
#include <coco/Vector4.hpp>


namespace coco {

/// @brief D-Pad with optional center button
/// Usage: gui.widget<Dpad>(id, haveButton);
class GuiDpad : public Gui::Widget {
public:
    struct Result {
        std::optional<bool> buttons[5];
    };

    /// @brief Constructor
    /// @param haveCenterButton True to add a clickable button in the center
    GuiDpad(bool haveCenterButton)
        : haveCenterButton(haveCenterButton) {}

    ~GuiDpad() override;

    /// @brief Draw and get state changes
    /// @param gui Gui object
    /// @return state changes
    Result update(Gui &gui);

    void touch(bool first, float x, float y) override;

    void release() override;

protected:
    // renderer for a dpad with center button
    class Dpad : public Gui::Renderer {
    public:
        Dpad();

        float2 draw(float2 position, float4 *colors);

    protected:
        GLint colorsUniform;
    };

    bool haveCenterButton;

    // current button states
    bool buttons[5] = {};
    bool lastButtons[5] = {};
};

} // namespace coco
