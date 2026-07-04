#include "renderer.hpp"

Renderer::Renderer() {}

Renderer::Renderer(shared_ptr<App> app) : m_app(app) {}

void Renderer::renderingProcess(int frameAccumulator){
    if (!isRendering()) return;

    m_progress = frameAccumulator;

    if (m_progress >= m_renderSamples) {
        m_isRendering = false;
        m_progress = m_renderSamples;

        m_app->exportImage(m_exportPath + m_exportFileName);
        if (!m_renderTextures) return;
        for (int i = 0; i < (int)m_renderingTextures.size(); i++){
            GLuint tex = m_renderingTextures[i];
            string name = m_renderingTexturesNames[i];
            m_app->exportTextureToExr(tex, m_exportPath + name + ".exr");
        }
    }
}

void Renderer::startRendering(int renderSamples, string exportFileName, bool fileDialog, bool renderTextures){
    m_isRendering = true;
    m_renderTextures = renderTextures;
    m_renderSamples = renderSamples;
    m_progress = 0;
    if (fileDialog){
        bool cancel;
        if (exportFileName.empty()){
            string path = m_app->saveFileDialog(cancel, "output.png");
            m_exportPath = "";
            m_exportFileName = path;
        }
        else{
            string path = m_app->pickFolderDialog(cancel);
            m_exportPath = path;
            m_exportFileName = exportFileName;    
        }
        if (cancel) cancelRender();
    }
    else{
        m_exportFileName = exportFileName;
    }
}

void Renderer::setRenderingTextures(vector<GLuint> textures, vector<string> names){
    m_renderingTextures = textures;
    m_renderingTexturesNames = names;
}

void Renderer::cancelRender(){
    m_isRendering = false;
    m_progress = 0;
}