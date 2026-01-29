#pragma once

#include "Gui.hpp"


namespace coco {

/// @brief Rotary button (incremental encoder knob with push button) on the emulator gui.
/// Usage: gui.widget<GuiRotaryButton>(id, haveCenterButton);
class GuiRotaryKnob : public Gui::Widget {
public:
    struct Result {
        std::optional<int> delta;
        std::optional<bool> button;
    };

    /// @brief Constructor
    /// @param increments Number of increments
    /// @param innerRadius Radius of an emulated button in the center (set to 0 to disable)
    /// @param haveCenterButton True to add a clickable button in the center
    GuiRotaryKnob(int increments, float innerRadius, bool haveCenterButton)
        : increments(increments), innerRadius(innerRadius), haveCenterButton(haveCenterButton), value(0), lastValue(0) {}

    ~GuiRotaryKnob() override;

    /// @brief Draw and get state changes
    /// @param gui Gui object
    /// @return state changes
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
        GLint incrementsUniform;
        GLint angleUniform;
        GLint outerColorUniform;
        GLint innerColorUniform;
    };

    int increments;
    float innerRadius;
    bool haveCenterButton;

    // current (mechanical) increment count in 16:16 format
    uint32_t value;
    uint32_t lastValue;

    // current button state
    bool button = false;
    bool lastButton = false;

    // last mouse position
    //float x = 0;
    //float y = 0;
    float2 lastPosition;
};

} // namespace coco
