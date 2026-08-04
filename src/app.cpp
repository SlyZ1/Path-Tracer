#include "app.hpp"
#define TINYEXR_IMPLEMENTATION
#include <tinyexr/tinyexr.h> 

using namespace std; 

GLFWwindow* App::Window = {};

void framebuffer_size_callback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

void App::init(int width, int height, const char *name, bool headless){
    if (!glfwInit())
    {
        cerr << "Failed to initialize GLFW" << endl;
        exit(1);
    }
    if (NFD_Init() != NFD_OKAY){
        printf("NFD_Init failed: %s\n", NFD_GetError());
        return;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    if (headless) glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    m_window = glfwCreateWindow(width, height, "ZMMR", NULL, NULL);
    Window = m_window;
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

    setDarkTitleBar(m_window);

    cudaGLSetGLDevice(0); 
    cudaFree(0);

    glViewport(0, 0, width, height);
    glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    m_io = &ImGui::GetIO(); (void)(*m_io);
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 430");

    glfwSwapInterval(0);
}

string App::saveFileDialog(bool& cancel, const string& defaultName){
    char* path;
    nfdsavedialogu8args_t args = {0};
    args.defaultName = defaultName.c_str();
    const nfdresult_t res = NFD_SaveDialogU8_With(&path, &args);
    switch (res) {
        case NFD_OKAY: {
            printf("NFD_SaveDialogU8_With success: %s\n", path);
            string result = path;
            NFD_FreePathU8(path);
            cancel = false;
            return result;
        }
        case NFD_CANCEL:
            printf("NFD_SaveDialogU8_With cancelled\n");
            break;
        case NFD_ERROR:
            printf("NFD_SaveDialogU8_With error: %s\n", NFD_GetError());
            break;
        default:
            break;
        }
    cancel = true;
    return "";
}

string App::openFileDialog(bool& cancel, nfdopendialogu8args_t args){
    char* path;
    const nfdresult_t res = NFD_OpenDialogU8_With(&path, &args);
    switch (res) {
        case NFD_OKAY: {
            printf("NFD_OpenDialogU8_With success: %s\n", path);
            string result = path;
            NFD_FreePathU8(path);
            cancel = false;
            return result;
        }
        case NFD_CANCEL:
            printf("NFD_OpenDialogU8_With cancelled\n");
            break;
        case NFD_ERROR:
            printf("NFD_OpenDialogU8_With error: %s\n", NFD_GetError());
            break;
        default:
            break;
        }
    cancel = true;
    return "";
}

string App::pickFolderDialog(bool& cancel){
    char* path;
    nfdpickfolderu8args_t args = {0};
    const nfdresult_t res = NFD_PickFolderU8_With(&path, &args);
    switch (res) {
        case NFD_OKAY: {
            printf("NFD_PickFolderU8_With success: %s\n", path);
            string result = path;
            result += "/";
            NFD_FreePathU8(path);
            cancel = false;
            return result;
        }
        case NFD_CANCEL:
            printf("NFD_PickFolderU8_With cancelled\n");
            break;
        case NFD_ERROR:
            printf("NFD_PickFolderU8_With error: %s\n", NFD_GetError());
            break;
        default:
            break;
    }
    cancel = true;
    return "";
}

void App::setClearColor(float r, float g, float b, float a){
    glClearColor(r, g, b, a);
}

void App::startFrame(unsigned int frameCount){
    bool altPressed = keyPressed(GLFW_KEY_LEFT_ALT);
    bool qPressedOnce = keyPressedOnce(GLFW_KEY_A, frameCount);
    if(altPressed && qPressedOnce)
        glfwSetWindowShouldClose(m_window, true);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void App::endFrame(bool swapBuffers){
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (swapBuffers) glfwSwapBuffers(m_window);
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

void App::setMousePos(float x, float y){
    glfwSetCursorPos(m_window, (double)x, (double)y);
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
    cout << "Program terminated." << endl;
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

void App::exportImage(const string& path){
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    vector<float> pixelsF(width() * height() * 3);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glReadPixels(
        0, 0,
        width(), height(),
        GL_RGB,
        GL_FLOAT,
        pixelsF.data()
    );

    vector<unsigned char> pixels(width() * height() * 3);
    for (unsigned int i = 0; i < width() * height() * 3; i++) {
        float c = std::clamp(pixelsF[i], 0.0f, 1.0f);
        pixels[i] = (unsigned char)(c * 255.0f + 0.5f);
    }
    unsigned char* data = pixels.data();

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

    lodepng::encode(path.c_str(), pixels, width(), height(), LCT_RGB);
}

void App::exportTextureToExr(GLuint tex, const string& path){
    if (tex == 0) return;

    int h = height();
    int w = width();

    vector<float> pixels(w * h * 4);
    glBindTexture(GL_TEXTURE_2D, tex);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    EXRHeader header;
    InitEXRHeader(&header);

    EXRImage image;
    InitEXRImage(&image);

    image.num_channels = 3;

    std::vector<float> images[3];
    images[0].resize(w * h);
    images[1].resize(w * h);
    images[2].resize(w * h);

    // Split RGBRGBRGB... into R, G and B layer
    for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
        int src = (y * w + x); 
        int dst = ((h - 1 - y) * w + x);
        images[0][dst] = pixels[4*src+0];
        images[1][dst] = pixels[4*src+1];
        images[2][dst] = pixels[4*src+2];
        }
    }

    float* image_ptr[3];
    image_ptr[0] = &(images[2].at(0)); // B
    image_ptr[1] = &(images[1].at(0)); // G
    image_ptr[2] = &(images[0].at(0)); // R

    image.images = (unsigned char**)image_ptr;
    image.width = w;
    image.height = h;

    header.num_channels = 3;
    header.channels = (EXRChannelInfo *)malloc(sizeof(EXRChannelInfo) * header.num_channels); 
    // Must be (A)BGR order, since most of EXR viewers expect this channel order.
    strncpy_s(header.channels[0].name, "B", 255); header.channels[0].name[strlen("B")] = '\0';
    strncpy_s(header.channels[1].name, "G", 255); header.channels[1].name[strlen("G")] = '\0';
    strncpy_s(header.channels[2].name, "R", 255); header.channels[2].name[strlen("R")] = '\0';

    header.pixel_types = (int *)malloc(sizeof(int) * header.num_channels); 
    header.requested_pixel_types = (int *)malloc(sizeof(int) * header.num_channels);
    for (int i = 0; i < header.num_channels; i++) {
      header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // pixel type of input image
      header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF; // pixel type of output image to be stored in .EXR
    }

    const char* err;
    int ret = SaveEXRImageToFile(&image, &header, path.c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
      fprintf(stderr, "Save EXR err: %s\n", err);
      return;
    }
    
    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);
    printf("Saved exr file. [ %s ] \n", path.c_str());
}