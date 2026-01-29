#include "Loop_emu.hpp"


namespace coco {

/// @brief Adds a new line to the emulator gui.
///
class Newline_emu : public Loop_emu::GuiHandler {
public:
    /// @param Constructor
    /// @param loop event loop
    Newline_emu(Loop_emu &loop);
    ~Newline_emu() override;

protected:
    void handle(Gui &gui) override;
};

} // namespace coco
