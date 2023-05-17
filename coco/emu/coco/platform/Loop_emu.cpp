#include "Loop_emu.hpp"
#include "Gui.hpp"
#include <iterator>
#include <iostream>


namespace coco {

namespace debug {

extern bool red;
extern bool green;
extern bool blue;

} // namespace debug



static void errorCallback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static void mouseCallback(GLFWwindow* window, int button, int action, int mods) {
 /*   if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if(GLFW_PRESS == action)
            lbutton_down = true;
        else if(GLFW_RELEASE == action)
            lbutton_down = false;
    }

    if(lbutton_down) {
         // do your drag here
    }*/
}


// Loop_emu

Loop_emu::Loop_emu() {
	// init GLFW
	glfwSetErrorCallback(errorCallback);
	if (!glfwInit())
		::exit(EXIT_FAILURE);

	// window size
	int width = 800;
	int height = 800;

	// scale window size on linux, is done automatically on mac
#ifdef __linux__
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	float xScale, yScale;
	glfwGetMonitorContentScale(monitor, &xScale, &yScale);

	width = int(width * xScale);
	height = int(height * yScale);
#endif

	// create GLFW window and OpenGL 3.3 Core context
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	//glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GL_TRUE);
	this->window = glfwCreateWindow(width, height, "CoCo", NULL, NULL);
	if (!this->window) {
		glfwTerminate();
		::exit(EXIT_FAILURE);
	}
	glfwSetKeyCallback(this->window, keyCallback);
	glfwSetMouseButtonCallback(this->window, mouseCallback);

	// make OpenGL context current
	glfwMakeContextCurrent(this->window);

	// load OpenGL functions
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	// v-sync
	glfwSwapInterval(1);
}

Loop_emu::~Loop_emu() {
	glfwDestroyWindow(this->window);
}

void Loop_emu::run(const int &condition) {
	int c = condition;
	Gui gui;
	while (c == condition && !glfwWindowShouldClose(this->window)) {
		//auto frameStart = std::chrono::steady_clock::now();

		// process events
		glfwPollEvents();
		handleEvents(0);

		// mouse
		gui.doMouse(this->window);

		// set viewport
		int width, height;
		glfwGetFramebufferSize(this->window, &width, &height);
		glViewport(0, 0, width, height);

		// clear screen
		glClear(GL_COLOR_BUFFER_BIT);

		// handle gui
		auto it = this->guiHandlers.begin();
		while (it != this->guiHandlers.end()) {

			// increment iterator beforehand because a yield handler can remove() itself
			auto &handler = *it;
			++it;

			handler.handle(gui);
		}

		// debug LEDs
		gui.newline();
		const int off = 0x202020;
		gui.draw<Led>(debug::red ? 0x0000ff : off);
		gui.draw<Led>(debug::green ? 0x00ff00 : off);
		gui.draw<Led>(debug::blue ? 0xff0000 : off);
		gui.draw<Led>((debug::red ? 0x0000ff : 0) | (debug::green ? 0x00ff00 : 0) | (debug::blue ? 0xff0000 : 0) | (!(debug::red | debug::green | debug::blue) ? off : 0));

		// swap render buffer to screen
		glfwSwapBuffers(this->window);

		// show frames per second
		/*auto now = std::chrono::steady_clock::now();
		++frameCount;
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
		if (duration.count() > 1000) {
			//std::cout << frameCount * 1000 / duration.count() << "fps" << std::endl;
			frameCount = 0;
			start = std::chrono::steady_clock::now();
		}*/
	}
}


// Loop_emu::GuiHandler

Loop_emu::GuiHandler::~GuiHandler() {
}

} // namespace coco
