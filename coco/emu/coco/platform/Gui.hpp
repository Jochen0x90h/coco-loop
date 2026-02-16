#pragma once

#include "glad/glad.h"
#include <GLFW/glfw3.h> // http://www.glfw.org/docs/latest/quick_guide.html
#include <coco/Font.hpp>
#include <map>
#include <optional>
#include <typeindex>
#include <unordered_map>
#include <vector>


namespace coco {

/// @brief Immetiate mode emulator user interface. The user interface has to be rebuilt every frame in the render loop
///
class Gui {
public:
    Gui();

    ~Gui() = default;

    /// @brief Handle mouse input and prepare for rendering
    ///
    void doMouse(GLFWwindow *window);

    /// @brief Advance "cursor" to next widget
    /// @param size Size of current widget
    void next(const float2 &size);

    /// @brief Start a new line of widgets
    ///
    void newline();


    /// @brief Draw an element that has neither a state nor mouse interaction such as a label or LED
    /// @tparam R renderer, a class that inherits Gui::Renderer
    template <typename R, typename... Args>
    float2 draw(Args... args) {
        Renderer *&renderer = this->renderers[std::type_index(typeid(R))];

        // create on first call
        if (renderer == nullptr) {
            glBindBuffer(GL_ARRAY_BUFFER, this->quadBuffer);
            renderer = new R();
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        // draw
        float2 size = static_cast<R *>(renderer)->draw(this->cursor, args...);

        // advance "cursor"
        next(size);
        //this->cursor.x += size.x + 0.01f;
        //this->maxHeight = std::max(this->maxHeight, size.y);

        return size;
    }

    void drawText(const TextureFont &font, const float2 &position, const float2 &scale, String text);

    /// @brief Add a widget to the gui
    /// @tname W widget, a class that inherits Gui::Widget
    /// @param id widget id
    template <typename W, typename... Args>
    auto widget(uint32_t id, Args... args) {
        // get widget and create if necessary
        W *widget = dynamic_cast<W*>(this->widgets[id]);
        if (widget == nullptr) {
            // delete in case it is a different type
            delete this->widgets[id];

            // create and set new widget
            widget = new W(args...);
            this->widgets[id] = widget;
        }

        // mark widget as used
        widget->used = true;

        // set position of widget
        widget->p1 = this->cursor;

        // draw, resize and generate result
        return widget->update(*this);
    }

    /// @brief Draw text on top of widget
    /// @param font Font
    /// @param id Widget id
    /// @param scale Font scale
    /// @param text Text to draw
    void drawText(const TextureFont &font, int id, const float2 &scale, String text);


    /// @brief render state
    ///
    class Renderer {
    public:

        Renderer(const char *fragmentShaderSource);

        void setState(const float2 &position, const float2 &size);

        void drawAndResetState();

        GLint getUniformLocation(const char *name) const {return glGetUniformLocation(this->program, name);}

    protected:

        GLuint program;
        GLint matLocation;
        GLuint vertexArray;
    };

    /// @brief Widget
    ///
    class Widget {
    public:
        virtual ~Widget();

        void resize(float2 size) {
            this->p2 = this->p1 + size;
        }

        virtual void touch(bool first, float x, float y) = 0;
        virtual void release() = 0;

        /**
            check if widget contains the given point
        */
        bool contains(float x, float y) const {
            return x >= this->p1.x && x <= this->p2.x && y >= this->p1.y && y <= this->p2.y;
        }

        // flag for garbage collection
        bool used = false;

        // bounding box
        float2 p1;
        float2 p2;
    };


    /// @brief Utility function for creating a texture
    /// @param filterMode GL_NEAREST or GL_LINEAR
    static GLuint createTexture(int filterMode);

protected:

    // vertex buffer containing a quad for drawing widgets
    GLuint quadBuffer;

    // renderers
    std::unordered_map<std::type_index, Renderer*> renderers;

    // widgets by id
    std::map<uint32_t, Widget*> widgets;
    Widget *activeWidget = nullptr;

    // "cursor" for placing widgets
    float2 cursor;
    float maxHeight;


    struct TextVertex {
        float2 position;
        float2 texcoord;
    };

    std::map<const TextureFont *, GLuint> fontTextures;
    GLuint textBuffer;
    GLint textProgram;
    GLuint textVertexArray;
    std::vector<TextVertex> textData;
};

} // namespace coco
