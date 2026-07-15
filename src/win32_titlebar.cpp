#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

void setDarkTitleBar(GLFWwindow* window){
    HWND hwnd = glfwGetWin32Window(window);
    COLORREF titleBarColor = RGB(51, 51, 51);
    DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &titleBarColor, sizeof(titleBarColor));
}