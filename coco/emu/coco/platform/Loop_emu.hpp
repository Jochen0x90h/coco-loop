#pragma once

#include "Gui.hpp"
#include <coco/Loop.hpp>
#include <coco/platform/Loop_native.hpp>


namespace coco {

/// @brief Extension of the native Loop implementation by a simlpe emulator user interface
///
class Loop_emu : public Loop_native {
public:

    Loop_emu();
    ~Loop_emu() override;

    void run() override;


    /// @brief Handler for graphical user interface of emulator.
    /// Update internal state and draw.
    class GuiHandler : public IntrusiveListNode {
    public:
        virtual ~GuiHandler();
        virtual void onGui(Gui &gui) = 0;
    };

    IntrusiveList<GuiHandler> guiHandlers;

protected:

    // opengl window
    GLFWwindow *window_ = nullptr;

};

} // namespace coco
