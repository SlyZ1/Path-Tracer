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

#ifndef APP_HPP
#define APP_HPP

using namespace std;

class App {
    private:
        GLFWwindow *m_window = {};
        ImGuiIO* m_io = {};
        bool m_cursorHidden = false;

    public:
        void init(int width, int height, const char *name);
        string saveFileDialog(bool& cancel);
        string pickFolderDialog(bool& cancel);
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
        ImGuiIO* getIo() { return m_io; }
};

#endif