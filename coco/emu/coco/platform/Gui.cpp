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
	static const float quadData[6 * 2] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};

	// create vertex buffer containing a quad
	glGenBuffers(1, &this->quadBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, this->quadBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadData), quadData, GL_STATIC_DRAW);

	// reset state
	glBindBuffer(GL_ARRAY_BUFFER, 0);


	// text rendering
	// --------------

	// create vertex buffer
	glGenBuffers(1, &this->textBuffer);

	// create vertex shader
	GLuint vertexShader = createShader(GL_VERTEX_SHADER,
		"#version 330\n"
		//"uniform mat4 mat;\n"
		"in vec2 position;\n"
		"in vec2 texcoord;\n"
		"out vec2 uv;\n"
		"void main() {\n"
		"	gl_Position = vec4(position, 0.0, 1.0);\n"
		"	uv = texcoord;\n"
		"}\n");

	// create fragment shader
	GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER,
		"#version 330\n"
		"uniform sampler2D tex;\n"
		"in vec2 uv;\n"
		"out vec4 pixel;\n"
		"void main() {\n"
		"pixel = texture(tex, uv).xxxw;\n"
		"}\n"
	);

	// create and link program
	int program = this->textProgram = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

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
	//this->matLocation = getUniformLocation("mat");

	// get shader inputs
	GLuint positionLocation = glGetAttribLocation(program, "position");
	GLuint texcoordLocation = glGetAttribLocation(program, "texcoord");

	// create vertex array objct (connects shader inputs to vertex buffers)
	glGenVertexArrays(1, &this->textVertexArray);
	glBindVertexArray(this->textVertexArray);
	glEnableVertexAttribArray(positionLocation);
	glEnableVertexAttribArray(texcoordLocation);
	glBindBuffer(GL_ARRAY_BUFFER, this->textBuffer);
	glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void*)offsetof(TextVertex, position));
	glVertexAttribPointer(texcoordLocation, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void*)offsetof(TextVertex, texcoord));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
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
			float localX = (windowX - this->activeWidget->p1.x) / (this->activeWidget->p2.x - this->activeWidget->p1.x);
			float localY = (windowY - this->activeWidget->p1.y) / (this->activeWidget->p2.y - this->activeWidget->p1.y);
			this->activeWidget->touch(first, localX, localY);
		}
	} else {
		if (this->activeWidget != nullptr)
			this->activeWidget->release();
		this->activeWidget = nullptr;
	}

	// prepare for rendering

	// reset screen coordinates
	this->cursor = {MARGIN, MARGIN};
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

void Gui::next(const float2 &size) {
	this->cursor.x += size.x + MARGIN;
	this->maxHeight = std::max(this->maxHeight, size.y);
}

void Gui::newline() {
	this->cursor.x = MARGIN;
	this->cursor.y += this->maxHeight + MARGIN;
	this->maxHeight = 0;
}

void Gui::drawText(const Font &font, const float2 &position, const float2 &scale, String text) {

	int textureWidth = font.dataSize & 0xffff;
	int textureHeight = font.dataSize >> 16;

	// create texture on first call
	GLuint &texture = this->fontTextures[&font];
	if (texture == 0) {
		texture = createTexture(GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, textureWidth, textureHeight, 0, GL_RED, GL_UNSIGNED_BYTE, font.data);
	}

	std::vector<TextVertex> &textData = this->textData;
	textData.clear();
	float x = position.x * 2.0 - 1.0f;
	float xScale = scale.x * 2.0;
	float yScale = scale.y * -2.0;
	float uScale = 1.0f / float(textureWidth);
	float vScale = 1.0f / float(textureHeight);
	for (auto info : font.glyphRange(text)) {
		if (!info.printable()) {
			// non-printable
			x += xScale * (info.width() + font.gapWidth);
		} else {
			// draw glyph
			auto glyph = info.textureGlyph();
			float y = 1.0 - position.y * 2.0 + yScale * glyph.y;
			float w = xScale * glyph.size.x;
			float h = yScale * glyph.size.y;
			float u1 = uScale * glyph.position.x;
			float v1 = vScale * glyph.position.y;
			float u2 = u1 + uScale * glyph.size.x;
			float v2 = v1 + vScale * glyph.size.y;

			TextVertex glyphData[6] = {
				{{x, y}, {u1, v1}},
				{{x + w, y}, {u2, v1}},
				{{x + w, y + h}, {u2, v2}},
				{{x, y}, {u1, v1}},
				{{x + w, y + h}, {u2, v2}},
				{{x, y + h}, {u1, v2}},
			};
			textData.insert(textData.end(), std::begin(glyphData), std::end(glyphData));

			// add character and gap width
			x += xScale * (glyph.size.x + font.gapWidth);
		}
	}

	// set state
	glBindBuffer(GL_ARRAY_BUFFER, this->textBuffer);
	glBufferData(GL_ARRAY_BUFFER, textData.size() * sizeof(TextVertex), textData.data(), GL_DYNAMIC_DRAW);
	glBindTexture(GL_TEXTURE_2D, texture);

	// use program
	glUseProgram(this->textProgram);

	// set vertex array
	glBindVertexArray(this->textVertexArray);

	// draw
	glDrawArrays(GL_TRIANGLES, 0, textData.size());

	// reset state
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Gui::drawText(const Font &font, int id, const float2 &scale, String text) {
	auto widget = this->widgets[id];

	float w = font.calcWidth(text) * scale.x;
	float h = font.height * scale.y;
	float2 position = (widget->p1 + widget->p2) * 0.5f - float2{w, h} * 0.5f;
	drawText(font, position, scale, text);
}

GLuint Gui::createTexture(int filterMode) {//int width, int height) {
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterMode);
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
	int program = this->program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

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

	// get shader inputs
	GLuint positionLocation = glGetAttribLocation(program, "position");

	// create vertex array objct (connects shader inputs to vertex buffers)
	glGenVertexArrays(1, &this->vertexArray);
	glBindVertexArray(this->vertexArray);
	glEnableVertexAttribArray(positionLocation);
	// vertex buffer is bound in Gui::draw() before calling this constructor
	glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glBindVertexArray(0);
}

void Gui::Renderer::setState(const float2 &position, const float2 &size) {
	// use program
	glUseProgram(this->program);

	// set matrix
	float mat[16] = {
		size.x * 2.0f, 0, 0, position.x * 2.0f - 1.0f,
		0, -size.y * 2.0f, 0, -(position.y * 2.0f - 1.0f),
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
/*
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

float2 Led::draw(float2 position, int color) {
	const float2 size = {0.025f, 0.025f};

	setState(position, size);
	float r = float(color & 0xff) / 255.0f;
	float g = float((color >> 8) & 0xff) / 255.0f;
	float b = float((color >> 16) & 0xff) / 255.0f;
	glUniform4f(this->colorUniform, r, g, b, 1.0f);
	drawAndResetState();

	return size;
}
*/
} // namespace coco
