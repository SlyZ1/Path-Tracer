#include <iostream>
#include <fstream>
#include <sstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include "app.hpp"
#include "shader_program.hpp"
#include "camera.hpp"
#include "mesh.hpp"
#include "ui.hpp"
#include "animator.hpp"
#include "scene.hpp"
#include "cuda_cpp/denoiser.hpp"

using namespace std;

#define SAMPLES 5

ShaderProgram rayTracingShader = {};
ShaderProgram accumulationShader = {};

GLuint VBO, VAO, EBO = 0;
GLuint FBO = 0;
GLuint texture = 0;
GLuint oldTexture = 0;
GLuint finalTexture = 0;
GLuint normalTexture = 0;
GLuint albedoTexture = 0;
GLuint colorTexture = 0;
GLuint depthTexture = 0;
GLuint denoisedTexture = 0;
GLuint texWidth, texHeight = 0;

int samples = 1;

int frameAccumulator = 0;
int frameCount = 0;

shared_ptr<Camera> camera = make_shared<Camera>(0.1f, 0.3f);
shared_ptr<App> app;
shared_ptr<Renderer> renderer;
shared_ptr<Animator> animator;
shared_ptr<UI> ui;
shared_ptr<Scene> scene;
shared_ptr<Denoiser> denoiser;

extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

void resetFrame(){
    frameAccumulator = 0;
}

void genTextures(unsigned int width, unsigned int height){
    glDeleteTextures(1, &texture);
    glDeleteTextures(1, &oldTexture);
    glDeleteTextures(1, &finalTexture);
    glDeleteTextures(1, &normalTexture);
    glDeleteTextures(1, &albedoTexture);
    glDeleteTextures(1, &colorTexture);
    glDeleteTextures(1, &depthTexture);
    glDeleteTextures(1, &denoisedTexture);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &oldTexture);
    glBindTexture(GL_TEXTURE_2D, oldTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &finalTexture);
    glBindTexture(GL_TEXTURE_2D, finalTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &normalTexture);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &albedoTexture);
    glBindTexture(GL_TEXTURE_2D, albedoTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &denoisedTexture);
    glBindTexture(GL_TEXTURE_2D, denoisedTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    resetFrame();
    texWidth = width;
    texHeight = height;
    vector<GLuint> textures = {finalTexture, albedoTexture, colorTexture, normalTexture, depthTexture};
    vector<string> texturesNames = {"reference", "albedo", "color", "normal", "depth"};
    renderer->setRenderingTextures(textures, texturesNames);
}

void initRayTraceShader(){
    rayTracingShader.use();
    glUniform2f(ShaderProgram::getVarLoc("winSize"), (float)app->width(), (float)app->height());
}

void reloadShaders(){
    rayTracingShader.reload(scene->getSpectral());
    initRayTraceShader();
    scene->updateScene();
    resetFrame();
    cout << "Shaders reloaded." << endl;
}

void init(bool headless = false){
    app = make_shared<App>();
    app->init(1600, 900, "Basic Raytracer", headless);
    app->setMousePos(app->width() / 2.0f, app->height() / 2.0f);
    app->toggleCursor(false);
    renderer = make_shared<Renderer>(app);
    // denoiser = make_shared<Denoiser>(512 + 256, 512 + 256, 9);
    // denoiser->load("denoiser.engine");
    
    accumulationShader.create();
    accumulationShader.load(GL_VERTEX_SHADER, "src/shaders/screenVertex.glsl");
    accumulationShader.load(GL_FRAGMENT_SHADER, "src/shaders/screenFrag.glsl");
    accumulationShader.link();

    rayTracingShader.create();
    rayTracingShader.load(GL_VERTEX_SHADER, "src/shaders/vertex.glsl");
    rayTracingShader.load(GL_FRAGMENT_SHADER, "src/shaders/ray.glsl");
    rayTracingShader.link();
    
    vector<float> vertices = {
        1.f,  1.f, 0.0f,
        1.f, -1.f, 0.0f,
        -1.f, -1.f, 0.0f,
        -1.f,  1.f, 0.0f
    };
    vector<unsigned int> indices = {
        0, 1, 3,
        1, 2, 3
    };  
    tie(VBO, VAO, EBO) = ShaderProgram::addData(vertices, indices);
    ShaderProgram::linkData(3, sizeof(float), 0);
    
    initRayTraceShader();
    
    glGenFramebuffers(1, &FBO);
    genTextures(app->width(), app->height());
    glDisable(GL_FRAMEBUFFER_SRGB);
    
    camera->resetMousePos(app->mouseX(), app->mouseY());
    scene = Scene::defaultScene(app, camera, resetFrame);
    animator = make_shared<Animator>(renderer, scene, resetFrame);
    ui = make_shared<UI>(app, renderer, animator, scene, camera, resetFrame);
    ui->setReloadShaderFunction(reloadShaders);
    // denoiser->init(albedoTexture, colorTexture, normalTexture, denoisedTexture);

    cout << "Program started." << endl;
}

void handleCamera(){
    if (!app->cursorIsHidden()){
        camera->hasStoppedMoving();
        return;
    }

    camera->move(
        app->keyPressed(GLFW_KEY_W), 
        app->keyPressed(GLFW_KEY_S), 
        app->keyPressed(GLFW_KEY_D), 
        app->keyPressed(GLFW_KEY_A),
        app->keyPressed(GLFW_KEY_SPACE),
        app->keyPressed(GLFW_KEY_LEFT_CONTROL),
        app->keyPressed(GLFW_KEY_LEFT_SHIFT)
    );
    camera->rotate(app->mouseX(), app->mouseY());
}

void render(){
    //Current frame
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, finalTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, normalTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, albedoTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, colorTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT5, GL_TEXTURE_2D, depthTexture, 0);
    GLenum drawBuffers[] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3,
        GL_COLOR_ATTACHMENT4,
        GL_COLOR_ATTACHMENT5,
    };
    glDrawBuffers(6, drawBuffers);
    rayTracingShader.use();
    
    ui->updateGPU();
    scene->updateGPU();
    camera->updateGPU();
    
    glUniform2f(ShaderProgram::getVarLoc("texSize"), (float)texWidth, (float)texHeight);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, oldTexture);
    glUniform1i(ShaderProgram::getVarLoc("screenTex"), 0);

    glUniform1i(ShaderProgram::getVarLoc("frameCount"), frameAccumulator);
    glUniform1i(ShaderProgram::getVarLoc("samples"), samples);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // glFinish();

    // denoiser->infer();

    // ShaderProgram::barrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    
    //Update oldTexture
    std::swap(texture, oldTexture);
    
    //Screen display (accumulation)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    accumulationShader.use();
    
    glActiveTexture(GL_TEXTURE0);
    GLuint renderTex = 0;
    switch(ui->textureDisplay){
        case UI::TexType::Color:
            renderTex = colorTexture;
            break;
        case UI::TexType::Albedo:
            renderTex = albedoTexture;
            break;
        case UI::TexType::Normal:
            renderTex = normalTexture;
            break;
        case UI::TexType::Depth:
            renderTex = depthTexture;
            break;
        case UI::TexType::Denoised:
            renderTex = denoisedTexture;
            break;
        case UI::TexType::Result:
            renderTex = finalTexture;
            break;
    }
    glBindTexture(GL_TEXTURE_2D, renderTex);
    glUniform1i(ShaderProgram::getVarLoc("oldTexture"), 0);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void inputs(){
    if (UI::isInteracting()) return;
    
    if (renderer->isRendering()) return;

    if (app->keyPressedOnce(GLFW_KEY_K, frameCount))
        cout << "Frame Time: " << glfwGetTime() << "s with " << frameAccumulator << " samples." << endl;
    
    if (app->keyPressedOnce(GLFW_KEY_ESCAPE, frameCount)){
        animator->cancelAnimation();
        renderer->cancelRender();

        app->setMousePos(app->width() / 2.0f, app->height() / 2.0f);
        ui->toggle();
        app->toggleCursor(app->cursorIsHidden());
        camera->resetMousePos(app->mouseX(), app->mouseY());
    }

    if (app->keyPressed(GLFW_KEY_LEFT_ALT) && app->keyPressedOnce(GLFW_KEY_S, frameCount)){
        bool cancel;
        string res = app->saveFileDialog(cancel, "output.png");
        if (!cancel){
            app->exportImage(res);
        }
    }

    if (app->keyPressedOnce(GLFW_KEY_R, frameCount)){
        reloadShaders();
    }

    if (app->mousePressedOnce(GLFW_MOUSE_BUTTON_LEFT, frameCount) && ui->isShowing() && !ui->isHovered()){
        glm::vec2 screenPos;
        screenPos.x = app->mouseX() / app->width();
        screenPos.y = app->mouseY() / app->height();
        screenPos = 2.0f * screenPos - glm::vec2(1.0f);
        screenPos.y *= -1;
        screenPos.x *= texWidth / (float)texHeight;
        Ray ray = Scene::rayFromClick(camera, screenPos);
        int primIndex = scene->intersectObject(ray);
        scene->selectObject(primIndex);
    }

    if (app->keyPressedOnce(GLFW_KEY_DELETE, frameCount)){
        scene->removeObject(scene->getSelectedObject());
    }

    if (app->keyPressed(GLFW_KEY_LEFT_CONTROL) && app->keyPressedOnce(GLFW_KEY_C, frameCount)){
        scene->copyObject(scene->getSelectedObject());
    }

    if (app->keyPressed(GLFW_KEY_LEFT_CONTROL) && app->keyPressedOnce(GLFW_KEY_V, frameCount)){
        int newIndex = scene->pasteObject();
        scene->selectObject(newIndex);
    }
}

void dynamicResolution(){
    if (camera->getIsMoving(frameCount) || (UI::isInteracting() && !renderer->isRendering())){
        resetFrame();
        int resolutionMultiplier = ui->getResolutionMultiplier();
        if (texWidth != app->width() / resolutionMultiplier)
            genTextures(app->width() / resolutionMultiplier, app->height() / resolutionMultiplier);
    }
    else if (texWidth != app->width()){
        resetFrame();
        genTextures(app->width(), app->height());
    }
}

void end(){
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteFramebuffers(1, &FBO);
    accumulationShader.destroy();
    rayTracingShader.destroy();
    scene->deleteScene();
    app->terminate();
}

int main(int argc, char* argv[]){
    bool headless = false;
    for (int i = 0; i < argc; i++) {
        if (string(argv[i]) == "headless") headless = true;
    }
    init(headless);
    while(!app->shouldClose())
    {
        app->startFrame(frameCount);
        if (!headless){
            handleCamera();
            ui->render(frameAccumulator);
        } 
        render();
        if (!headless){
            inputs();
            animator->animationProcess();
            renderer->renderingProcess(frameAccumulator);
        }
        
        frameAccumulator += samples;
        frameCount++;
        if (!headless) dynamicResolution();
        app->endFrame();
    }
    end();
    return EXIT_SUCCESS;
}