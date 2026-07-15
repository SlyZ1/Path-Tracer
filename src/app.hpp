#ifndef APP_HPP
#define APP_HPP

#include <iostream>
#include <lodepng/lodepng.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_glfw.h>
#include <glad/glad.h>
#define GLFW_NATIVE_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <nativefiledialog/nfd.h>
#include <cuda_gl_interop.h>
#include <vector>
#include "win32_titlebar.hpp"

using namespace std;

class App {
    private:
        GLFWwindow *m_window = {};
        ImGuiIO* m_io = {};
        bool m_cursorHidden = false;

    public:
        static GLFWwindow *Window;

        void init(int width, int height, const char *name, bool headless=false);
        string saveFileDialog(bool& cancel, const string& defaultName);
        string openFileDialog(bool& cancel, nfdopendialogu8args_t args = {0});
        string pickFolderDialog(bool& cancel);
        GLFWwindow* getWindow() const {return m_window; };
        void setClearColor(float r, float g, float b, float a);
        void startFrame(unsigned int frameCount);
        void endFrame();
        bool shouldClose();
        bool keyPressed(int key);
        bool keyPressedOnce(int key, unsigned int frame);
        bool mousePressed(int button);
        bool mousePressedOnce(int button, unsigned int frame);
        void setMousePos(float x, float y);
        void toggleCursor(bool show);
        bool cursorIsHidden();
        float mouseX();
        float mouseY();
        void terminate();
        unsigned int width();
        unsigned int height();
        void exportImage(const string& path = "outputs/image.png");
        void exportTextureToExr(GLuint tex, const string& path = "outputs/image.png");
};

#endif