#include "Loop_emu.hpp"
#include "Gui.hpp"
#include "GuiDpad.hpp"
#include "GuiLed.hpp"
#include "GuiRotaryKnob.hpp"
#include <iterator>
#include <iostream>
#include "font/tahoma16pt8bpp.hpp"


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

Loop_emu::Loop_emu() : Loop_native(true) {
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
    window_ = glfwCreateWindow(width, height, "CoCo", NULL, NULL);
    if (!window_) {
        glfwTerminate();
        ::exit(EXIT_FAILURE);
    }
    glfwSetKeyCallback(window_, keyCallback);
    glfwSetMouseButtonCallback(window_, mouseCallback);

    // make OpenGL context current
    glfwMakeContextCurrent(window_);

    // load OpenGL functions
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // v-sync
    glfwSwapInterval(1);
}

Loop_emu::~Loop_emu() {
    glfwDestroyWindow(window_);
}

void Loop_emu::run() {
    Gui gui;
    while (!exitFlag_ && !glfwWindowShouldClose(window_)) {
        //auto frameStart = std::chrono::steady_clock::now();

        // process events
        glfwPollEvents();
        handleEvents(0);

        // mouse
        gui.doMouse(window_);

        // set viewport
        int width, height;
        glfwGetFramebufferSize(window_, &width, &height);
        glViewport(0, 0, width, height);

        // clear screen
        glClear(GL_COLOR_BUFFER_BIT);

        // handle gui
        auto it = guiHandlers.begin();
        while (it != guiHandlers.end()) {

            // increment iterator beforehand because a handler can remove() itself
            auto &handler = *it;
            ++it;

            handler.onGui(gui);
        }

        // debug LEDs
        gui.newline();
        const int off = 0x202020;
        gui.draw<GuiLed>(debug::red ? 0x0000ff : off);
        gui.draw<GuiLed>(debug::green ? 0x00ff00 : off);
        gui.draw<GuiLed>(debug::blue ? 0xff0000 : off);
        gui.draw<GuiLed>((debug::red ? 0x0000ff : 0) | (debug::green ? 0x00ff00 : 0) | (debug::blue ? 0xff0000 : 0) | (!(debug::red | debug::green | debug::blue) ? off : 0));

        // test
        /*
        gui.drawText(tahoma16pt8bpp, {0, 0.5}, {0.002, 0.002}, "Ω012345\r");
        auto result = gui.widget<GuiRotaryKnob>(100, // id
            24, // number of increments
            0.1, // inner radius
            true); // with center button
        */
        /*
        auto result = gui.widget<GuiDpad>(101, // id
            true); // with center button
        int i = 0;
        for (auto button : result.buttons) {
            if (button)
                std::cout << i << ' ' << (*button ? "pressed" : "released") << std::endl;
            ++i;
        }
        */

        // swap render buffer to screen
        glfwSwapBuffers(window_);

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
    exitFlag_ = false;
}


// Loop_emu::GuiHandler

Loop_emu::GuiHandler::~GuiHandler() {
}

} // namespace coco
