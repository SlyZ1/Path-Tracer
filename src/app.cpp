#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "app.hpp"

using namespace std;

void framebuffer_size_callback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

void App::init(int width, int height, const char *name){
    if (!glfwInit())
    {
        cerr << "Failed to initialize GLFW" << endl;
        exit(1);
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    m_window = glfwCreateWindow(width, height, "ZMMR", NULL, NULL);
    glfwSetWindowTitle(m_window, name);
    if (m_window == NULL)
    {
        cerr << "Failed to open GLFW window" << endl;
        exit(1);
    }
    glfwMakeContextCurrent(m_window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cerr << "Failed to initialize GLAD" << endl;
        exit(1);
    }

    glViewport(0, 0, width, height);
    glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    m_io = &ImGui::GetIO(); (void)(*m_io);
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 430");
}

void App::setClearColor(float r, float g, float b, float a){
    glClearColor(r, g, b, a);
}

void App::startFrame(unsigned int frameCount){
    bool altPressed = keyPressed(GLFW_KEY_LEFT_ALT);
    bool qPressedOnce = keyPressedOnce(GLFW_KEY_A, frameCount);
    if(altPressed && qPressedOnce)
        glfwSetWindowShouldClose(m_window, true);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void App::endFrame(){
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(m_window);
    glfwPollEvents();
}

bool App::shouldClose(){
    return glfwWindowShouldClose(m_window);
}

bool App::keyPressed(int key){
    return glfwGetKey(m_window, key) == GLFW_PRESS;
}

bool App::keyPressedOnce(int key, unsigned int frame){
    static unsigned int wasPressed[GLFW_KEY_LAST + 1] = {UINT_MAX};

    bool isPressed = glfwGetKey(m_window, key) == GLFW_PRESS;

    if (isPressed && wasPressed[key] >= frame) {
        wasPressed[key] = frame;
        return true;
    }

    if (!isPressed) {
        wasPressed[key] = UINT_MAX;
    }

    return false;
}

bool App::mousePressed(int button){
    return glfwGetMouseButton(m_window, button) == GLFW_PRESS;
}

bool App::mousePressedOnce(int button, unsigned int frame){
    static unsigned int wasPressed[GLFW_MOUSE_BUTTON_LAST + 1] = {UINT_MAX};

    bool isPressed = glfwGetMouseButton(m_window, button) == GLFW_PRESS;

    if (isPressed && wasPressed[button] >= frame) {
        wasPressed[button] = frame;
        return true;
    }

    if (!isPressed) {
        wasPressed[button] = UINT_MAX;
    }

    return false;
}

void App::toggleCursor(bool show){
    m_cursorHidden = !show;
    glfwSetInputMode(m_window, GLFW_CURSOR, !show ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

bool App::cursorIsHidden(){
    return m_cursorHidden;
}

float App::mouseX(){
    double mouseX;
    glfwGetCursorPos(m_window, &mouseX, nullptr);
    return static_cast<float>(mouseX);
}

float App::mouseY(){
    double mouseY;
    glfwGetCursorPos(m_window, nullptr, &mouseY);
    return static_cast<float>(mouseY);
}

void App::terminate(){
    glfwTerminate();
}

unsigned int App::width(){
    int width;
    glfwGetWindowSize(m_window, &width, nullptr);
    return width;
}

unsigned int App::height(){
    int height;
    glfwGetWindowSize(m_window, nullptr, &height);
    return height;
}

void App::exportImage(const string& additionalPath, const string& name){
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    vector<unsigned char> pixels(width() * height() * 3);
    auto data = pixels.data();

    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glReadPixels(
        0, 0,
        width(), height(),
        GL_RGB,
        GL_UNSIGNED_BYTE,
        data
    );

    //flip vertical
    int stride = width() * 3;
    vector<unsigned char> row(stride);

    for (unsigned int y = 0; y < height() / 2; y++) {
        unsigned char* row1 = data + y * stride;
        unsigned char* row2 = data + (height() - y - 1) * stride;

        memcpy(row.data(), row1, stride);
        memcpy(row1, row2, stride);
        memcpy(row2, row.data(), stride);
    }

    string path = "outputs/" + additionalPath + name;
    lodepng::encode(path.c_str(), pixels, width(), height(), LCT_RGB);
}