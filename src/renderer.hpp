#ifndef RENDERER
#define RENDERER
#include "app.hpp"
#include <memory>
#include <filesystem>

using namespace std;
namespace fs = filesystem;

class Renderer {
private:
    bool m_isRendering = false;
    bool m_renderTextures = false;
    int m_progress = 0;
    int m_renderSamples = 0;
    shared_ptr<App> m_app = {};
    string m_exportPath = {};
    string m_exportFileName = {};
    vector<GLuint> m_renderingTextures = {};
    vector<string> m_renderingTexturesNames = {};
public:
    Renderer();
    Renderer(shared_ptr<App> app);
    bool isRendering() const { return m_isRendering; }
    int getProgress() const { return m_progress; }
    void startRendering(int renderSamples, string exportFileName = "", bool fileDialog = true, bool renderTextures = false);
    void setRenderingTextures(vector<GLuint> textures, vector<string> names);
    void renderingProcess(int frameAccumulator);
    void cancelRender();
};

#endif