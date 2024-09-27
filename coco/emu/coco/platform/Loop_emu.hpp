#pragma once

#include "Gui.hpp"
#include <coco/Loop.hpp>
#include <coco/platform/Loop_native.hpp>


namespace coco {

/**
	Extension of the native Loop implementation by a simlpe emulator user interface
*/
class Loop_emu : public Loop_native {
public:

	Loop_emu();
	~Loop_emu() override;

	void run() override;


	class GuiHandler : public IntrusiveListNode {
	public:
		virtual ~GuiHandler();
		virtual void handle(Gui &gui) = 0;
	};

	IntrusiveList<GuiHandler> guiHandlers;

protected:

	// opengl window
	GLFWwindow *window = nullptr;

};

} // namespace coco
