#ifndef RENDERER
#define RENDERER
#include "app.hpp"
#include <memory>

using namespace std;

class Renderer {
private:
    bool m_is_rendering = false;
    int m_progress = 0;
    int m_renderSamples = 0;
    shared_ptr<App> m_app = {};
    string m_exportPath = {};
    string m_exportFileName = {};
public:
    Renderer();
    Renderer(shared_ptr<App> app);
    bool isRendering() const { return m_is_rendering; }
    int getProgress() const { return m_progress; }
    void startRendering(int renderSamples, string exportFileName, bool fileDialog = true);
    void renderingProcess(int frameAccumulator);
    void cancelRender();
};

#endif