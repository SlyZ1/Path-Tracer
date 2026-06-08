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

using namespace std;

#define SAMPLES 5

ShaderProgram rayTraceShader;
ShaderProgram accumulationShader;

unsigned int VBO, VAO, EBO;
unsigned int FBO;
unsigned int texture;
unsigned int oldTexture;
unsigned int texWidth, texHeight;

int samples = 1;

int frameAccumulator = 0;
int frameCount = 0;

Camera camera(0.1, 0.3);
shared_ptr<App> app;
shared_ptr<Renderer> renderer;
shared_ptr<Animator> animator;
shared_ptr<UI> ui;
shared_ptr<Scene> scene;

extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

void resetFrame(){
    frameAccumulator = 0;
}

void genTexture(unsigned int width, unsigned int height){
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

    resetFrame();
    texWidth = width;
    texHeight = height;
}

void init(){
    app = make_shared<App>();
    app->init(1600, 900, "Basic Raytracer");
    app->toggleCursor(false);
    renderer = make_shared<Renderer>(app);
    animator = make_shared<Animator>(renderer, resetFrame);
    
    rayTraceShader.create();
    rayTraceShader.load(GL_VERTEX_SHADER, "src/shaders/vertex.glsl");
    rayTraceShader.load(GL_FRAGMENT_SHADER, "src/shaders/frag.glsl");
    rayTraceShader.link();
    
    accumulationShader.create();
    accumulationShader.load(GL_VERTEX_SHADER, "src/shaders/screenVertex.glsl");
    accumulationShader.load(GL_FRAGMENT_SHADER, "src/shaders/screenFrag.glsl");
    accumulationShader.link();
    
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
    
    rayTraceShader.use();
    int winSizeLoc = ShaderProgram::getVarLoc("winSize");
    glUniform2f(winSizeLoc, app->width(), app->height());
    
    scene = make_shared<Scene>(Scene::defaultScene(app, resetFrame));
    
    Mesh* mesh = new Mesh();
    mesh->loadFromModel("models/Cube.obj");
    vector<Triangle> triangles = mesh->getTriangles();
    cout << "Loaded model with " << triangles.size() << " triangles." << endl;
    GLuint trissbo;
    glGenBuffers(1, &trissbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, trissbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, triangles.size() * sizeof(Triangle), triangles.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, trissbo); // binding 0
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glUniform1i(ShaderProgram::getVarLoc("numTriangles"), triangles.size());
    
    vector<int> triIndices(triangles.size());
    int size = triangles.size();
    for (int i = 0; i < size; i++) {
        triIndices[i] = i;
    }
    BVHNode* bvh = Mesh::computeBVH(triangles, triIndices, 0, triangles.size());
    vector<linBVHNode> linNodes = Mesh::lineariseBVH(bvh, triangles);
    GLuint bvhssbo;
    glGenBuffers(1, &bvhssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bvhssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, linNodes.size() * sizeof(linBVHNode), linNodes.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, bvhssbo); // binding 1
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glUniform1i(ShaderProgram::getVarLoc("numBVHNodes"), linNodes.size());
    
    glGenFramebuffers(1, &FBO);
    genTexture(app->width(), app->height());
    glDisable(GL_FRAMEBUFFER_SRGB);
    
    camera.resetMousePos(app->mouseX(), app->mouseY());
    ui = make_shared<UI>(app, renderer, animator, scene, resetFrame);
}

void handleCamera(){
    if (!app->cursorIsHidden()){
        camera.hasStoppedMoving();
        return;
    }

    camera.move(
        app->keyPressed(GLFW_KEY_W), 
        app->keyPressed(GLFW_KEY_S), 
        app->keyPressed(GLFW_KEY_D), 
        app->keyPressed(GLFW_KEY_A),
        app->keyPressed(GLFW_KEY_SPACE),
        app->keyPressed(GLFW_KEY_LEFT_CONTROL),
        app->keyPressed(GLFW_KEY_LEFT_SHIFT)
    );
    camera.rotate(app->mouseX(), app->mouseY());
}

void render(){
    //Current frame
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    rayTraceShader.use();
    
    ui->updateGPU();
    scene->updateGPU();
    
    glUniform2f(ShaderProgram::getVarLoc("texSize"), texWidth, texHeight);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, oldTexture);
    glUniform1i(ShaderProgram::getVarLoc("screenTex"), 0);
    glUniform1i(ShaderProgram::getVarLoc("frameCount"), frameAccumulator);
    glUniform1i(ShaderProgram::getVarLoc("samples"), samples);
    
    glUniform3f(ShaderProgram::getVarLoc("camera.pos"), camera.position().x, camera.position().y, camera.position().z);
    glUniform3f(ShaderProgram::getVarLoc("camera.lookDir"), camera.lookDir().x, camera.lookDir().y, camera.lookDir().z);
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    //Update oldTexture
    std::swap(texture, oldTexture);
    
    //Screen display (accumulation)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    accumulationShader.use();
    
    glBindTexture(GL_TEXTURE_2D, oldTexture);
    glUniform1i(ShaderProgram::getVarLoc("oldTexture"), 0);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void inputs(){
    if (UI::isInteracting()) return;
    
    if (app->keyPressedOnce(GLFW_KEY_ESCAPE, frameCount)){
        animator->cancelAnimation();
        renderer->cancelRender();
    }
    
    if (renderer->isRendering()) return;

    if (app->keyPressedOnce(GLFW_KEY_K, frameCount))
        cout << "Frame Time: " << glfwGetTime() << "s with " << frameAccumulator << " samples." << endl;
    
    if (app->keyPressedOnce(GLFW_KEY_ESCAPE, frameCount)){
        ui->toggle();
        app->toggleCursor(app->cursorIsHidden());
        camera.resetMousePos(app->mouseX(), app->mouseY());
    }

    if (app->keyPressedOnce(GLFW_KEY_ENTER, frameCount) && !ui->isShowing()){
        Primitive newprim;
        newprim.type = PrimType::SPHERE;
        newprim.pos = glm::vec3(rand() / (float)RAND_MAX * 10, 1, rand() / (float)RAND_MAX * 10);
        newprim.scale = rand() / (float)RAND_MAX + 0.5;
        newprim.mat = Scene::glassMaterial(glm::vec3(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX), rand() / (float)RAND_MAX*2 + 1);
        scene->addObject(newprim);
    }

    if (app->keyPressed(GLFW_KEY_LEFT_ALT) && app->keyPressedOnce(GLFW_KEY_S, frameCount)){
        bool cancel;
        string res = app->saveFileDialog(cancel);
        if (!cancel) app->exportImage(res);
    }

    if (app->mousePressedOnce(GLFW_MOUSE_BUTTON_LEFT, frameCount) && ui->isShowing() && !ui->isHovered()){
        glm::vec2 screenPos;
        screenPos.x = app->mouseX() / app->width();
        screenPos.y = app->mouseY() / app->height();
        screenPos = 2.0f * screenPos - glm::vec2(1.0f);
        screenPos.y *= -1;
        screenPos.x *= texWidth / (float)texHeight;
        Ray ray = Scene::rayFromClick(camera.position(), camera.lookDir(), screenPos);
        int primIndex = scene->intersectObject(ray);
        scene->selectPrimitive(primIndex);
    }

    if (app->keyPressedOnce(GLFW_KEY_DELETE, frameCount)){
        scene->removeObject(scene->getSelectedPrimitive());
    }

    if (app->keyPressed(GLFW_KEY_LEFT_CONTROL) && app->keyPressedOnce(GLFW_KEY_C, frameCount)){
        scene->copyPrimitive(scene->getSelectedPrimitive());
    }

    if (app->keyPressed(GLFW_KEY_LEFT_CONTROL) && app->keyPressedOnce(GLFW_KEY_V, frameCount)){
        int newIndex = scene->pastePrimitive();
        scene->selectPrimitive(newIndex);
    }
}

void dynamicResolution(){
    if (camera.getIsMoving(frameCount) || (UI::isInteracting() && !renderer->isRendering())){
        resetFrame();
        int resolutionMultiplier = ui->getResolutionMultiplier();
        if (texWidth != app->width() / resolutionMultiplier) 
            genTexture(app->width() / resolutionMultiplier, app->height() / resolutionMultiplier);
    }
    else if (texWidth != app->width()){
        resetFrame();
        genTexture(app->width(), app->height());
    }
}

void end(){
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteFramebuffers(1, &FBO);
    rayTraceShader.destroy();
    app->terminate();
}

int main(){
    init();
    while(!app->shouldClose())
    {
        app->startFrame(frameCount);
        handleCamera();
        
        ui->render();
        render();
        inputs();

        animator->animationProcess();
        renderer->renderingProcess(frameAccumulator);
        
        frameAccumulator += samples;
        frameCount++;
        dynamicResolution();
        app->endFrame();
    }
    end();
    return EXIT_SUCCESS;
}