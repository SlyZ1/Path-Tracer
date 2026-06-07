#include "renderer.hpp"

Renderer::Renderer() {}

Renderer::Renderer(shared_ptr<App> app) : m_app(app) {}

void Renderer::renderingProcess(int frameAccumulator){
    if (!isRendering()) return;

    m_progress = frameAccumulator;

    if (m_progress >= m_renderSamples) {
        m_is_rendering = false;
        m_progress = m_renderSamples;
        m_app->exportImage(m_exportPath + m_exportFileName);
    }
}

void Renderer::startRendering(int renderSamples, string exportFileName, bool fileDialog){
    m_is_rendering = true;
    m_renderSamples = renderSamples;
    m_progress = 0;
    if (fileDialog){
        bool cancel;
        string path = m_app->pickFolderDialog(cancel);
        m_exportPath = path;
        m_exportFileName = exportFileName;
        if (cancel) cancelRender();
    }
}

void Renderer::cancelRender(){
    m_is_rendering = false;
    m_progress = 0;
}