#include "Gui.hpp"
#include <vector>
#include <stdexcept>
#include <cmath>
#include <iostream>


namespace coco {

float const MARGIN = 0.02f;

static GLuint createShader(GLenum type, char const *code) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &code, nullptr);
	glCompileShader(shader);

	// check compile status
	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		// get length of log (including trailing null character)
		GLint length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

		// get log
		std::vector<char> errorLog(length);
		glGetShaderInfoLog(shader, length, &length, (GLchar*)errorLog.data());

		throw std::runtime_error(errorLog.data());
	}
	return shader;
}


// Gui

Gui::Gui() {
	static const float quadData[6*2] =
	{
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};

	// create vertex buffer containing a quad
	glGenBuffers(1, &this->quad);
	glBindBuffer(GL_ARRAY_BUFFER, this->quad);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadData), quadData, GL_STATIC_DRAW);

	// reset state
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Gui::doMouse(GLFWwindow *window) {
	// get window size
	int windowWidth, windowHeight;
	glfwGetWindowSize(window, &windowWidth, &windowHeight);

	double x;
	double y;
	glfwGetCursorPos(window, &x, &y);
	bool leftDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
	//std::cout << "x " << x << " y " << y << " down " << leftDown << std::endl;

	if (leftDown) {
		bool first = this->activeWidget == nullptr;
		float windowX = x / windowWidth;
		float windowY = y / windowHeight;
		if (first) {
			// search widget under mouse
			for (auto && p : this->widgets) {
				Widget *widget = p.second;
				if (widget->contains(windowX, windowY))
					this->activeWidget = widget;
			}
		}
		if (this->activeWidget != nullptr) {
			float localX = (windowX - this->activeWidget->x1) / (this->activeWidget->x2 - this->activeWidget->x1);
			float localY = (windowY - this->activeWidget->y1) / (this->activeWidget->y2 - this->activeWidget->y1);
			this->activeWidget->touch(first, localX, localY);
		}
	} else {
		if (this->activeWidget != nullptr)
			this->activeWidget->release();
		this->activeWidget = nullptr;
	}

	// prepare for rendering

	// reset screen coordinates
	this->x = MARGIN;
	this->y = MARGIN;
	this->maxHeight = 0;

	// garbage collect unused widgets
	auto it = this->widgets.begin();
	while (it != this->widgets.end()) {
		auto next = it;
		++next;

		if (!it->second->used)
			this->widgets.erase(it);
		else
			it->second->used = false;

		it = next;
	}
}

void Gui::next(float w, float h) {
	this->x += w + MARGIN;
	this->maxHeight = std::max(this->maxHeight, h);
}

void Gui::newline() {
	this->x = MARGIN;
	this->y += this->maxHeight + MARGIN;
	this->maxHeight = 0;
}

GLuint Gui::createTexture() {//int width, int height) {
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);
	return texture;
}


// Gui::Renderer

Gui::Renderer::Renderer(char const *fragmentShaderSource) {
	// create vertex shader
	GLuint vertexShader = createShader(GL_VERTEX_SHADER,
		"#version 330\n"
		"uniform mat4 mat;\n"
		"in vec2 position;\n"
		"out vec2 xy;\n"
		"void main() {\n"
		"	gl_Position = mat * vec4(position, 0.0, 1.0);\n"
		"	xy = position;\n"
		"}\n");

	// create fragment shader
	GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

	// create and link program
	this->program = glCreateProgram();
	glAttachShader(this->program, vertexShader);
	glAttachShader(this->program, fragmentShader);
	glLinkProgram(this->program);

	// check link status
	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		// get length of log (including trailing null character)
		GLint length;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

		// get log
		std::vector<char> errorLog(length);
		glGetProgramInfoLog(program, length, &length, (GLchar*)errorLog.data());

		throw std::runtime_error(errorLog.data());
	}

	// get uniform locations
	this->matLocation = getUniformLocation("mat");

	// create vertex array objct
	GLuint positionLocation = glGetAttribLocation(this->program, "position");
	glGenVertexArrays(1, &this->vertexArray);
	glBindVertexArray(this->vertexArray);
	glEnableVertexAttribArray(positionLocation);
	// vertex buffer is already bound
	glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glBindVertexArray(0);
}

void Gui::Renderer::setState(float x, float y, float w, float h) {
	// use program
	glUseProgram(this->program);

	// set matrix
	float mat[16] = {
		w * 2.0f, 0, 0, x * 2.0f - 1.0f,
		0, -h * 2.0f, 0, -(y * 2.0f - 1.0f),
		0, 0, 0, 0,
		0, 0, 0, 1};
	glUniformMatrix4fv(this->matLocation, 1, true, mat);

	// set vertex array
	glBindVertexArray(this->vertexArray);
}

void Gui::Renderer::drawAndResetState() {
	// draw
	glDrawArrays(GL_TRIANGLES, 0, 6);

	// reset
	glBindVertexArray(0);
}


// Gui::Widget

Gui::Widget::~Widget() {
}


// Led

Led::Led()
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

Gui::Size Led::draw(float x, float y, int color) {
	const float w = 0.025f;
	const float h = 0.025f;

	setState(x, y, w, h);
	float r = float(color & 0xff) / 255.0f;
	float g = float((color >> 8) & 0xff) / 255.0f;
	float b = float((color >> 16) & 0xff) / 255.0f;
	glUniform4f(this->colorUniform, r, g, b, 1.0f);
	drawAndResetState();

	return {w, h};
}

} // namespace coco
