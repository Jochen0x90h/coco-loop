#pragma once

#include "glad/glad.h"
#include <GLFW/glfw3.h> // http://www.glfw.org/docs/latest/quick_guide.html
#include <coco/assert.hpp>
#include <map>
#include <unordered_map>
#include <typeindex>
#include <optional>


namespace coco {

/**
	Immetiate mode emulator user interface. The user interface has to be rebuilt every frame in the render loop
*/
class Gui {
public:

	struct Size {
		float w;
		float h;
	};

	Gui();

	~Gui() = default;

	/**
		Handle mouse input and prepare for rendering
	*/
	void doMouse(GLFWwindow *window);

	void next(float w, float h);

	/**
		Put the following gui elements onto a new line
	*/
	void newline();


	/**
		Draw an element that has neither a state nor mouse interaction such as a label or LED
		@tname R renderer, a class that inherits Gui::Renderer
	*/
	template <typename R, typename... Args>
	Size draw(Args... args) {
		Renderer *& renderer = this->renderers[std::type_index(typeid(R))];

		// create on first call
		if (renderer == nullptr) {
			glBindBuffer(GL_ARRAY_BUFFER, this->quad);
			renderer = new R();
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		// draw
		Size size = static_cast<R *>(renderer)->draw(this->x, this->y, args...);

		// advance "cursor"
		this->x += size.w + 0.01f;
		this->maxHeight = std::max(this->maxHeight, size.h);

		return size;
	}

	/**
		Add a widget to the gui
		@tname W widget, a class that inherits Gui::Widget
	*/
	template <typename W, typename... Args>
	auto widget(uint32_t id, Args... args) {
		// get widget and create if necessary
		W *widget = dynamic_cast<W*>(this->widgets[id]);
		if (widget == nullptr) {
			delete this->widgets[id];
			widget = new W(args...);
			this->widgets[id] = widget;
		}

		// mark as used
		widget->used = true;

		// set position
		widget->x1 = this->x;
		widget->y1 = this->y;

		// draw, resize and generate result
		return widget->update(*this);
	}


	// render state
	class Renderer {
	public:

		Renderer(const char *fragmentShaderSource);

		void setState(float x, float y, float w, float h);

		void drawAndResetState();

		GLint getUniformLocation(const char *name) const {return glGetUniformLocation(this->program, name);}

	protected:

		GLuint program;
		GLint matLocation;
		GLuint vertexArray;
	};

	// widget
	class Widget {
	public:
		virtual ~Widget();

		void resize(Size size) {
			this->x2 = this->x1 + size.w;
			this->y2 = this->y1 + size.h;
		}

		virtual void touch(bool first, float x, float y) = 0;
		virtual void release() = 0;

		/**
			check if widget contains the given point
		*/
		bool contains(float x, float y) const {
			return x >= this->x1 && x <= this->x2 && y >= this->y1 && y <= this->y2;
		}

		// flag for garbage collection
		bool used = false;

		// bounding box
		float x1;
		float y1;
		float x2;
		float y2;
	};


	// utility function
	static GLuint createTexture();

protected:

	// vertex buffer containing a quad for drawing widgets
	GLuint quad;

	// renderers
	std::unordered_map<std::type_index, Renderer*> renderers;

	// widgets by id
	std::map<uint32_t, Widget*> widgets;
	Widget *activeWidget = nullptr;

	// "cursor" for placing widgets
	float x;
	float y;
	float maxHeight;
};


/**
	Debug LED on the emulator gui.
	Usage: gui.draw<LED>(color);
*/
class Led : public Gui::Renderer {
public:
	Led();

	Gui::Size draw(float x, float y, int color);

protected:
	GLint colorUniform;
};

} // namespace coco
