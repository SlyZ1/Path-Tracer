#include "renderer.hpp"

Renderer::Renderer() {}

Renderer::Renderer(shared_ptr<App> app) : m_app(app) {}

void Renderer::renderingProcess(int frameAccumulator){
    if (!isRendering()) return;

    m_progress = frameAccumulator;

    if (m_progress >= m_renderSamples) {
        m_is_rendering = false;
        m_progress = m_renderSamples;
        m_app->exportImage(m_export_folder, m_export_file);
    }
}

void Renderer::startRendering(int renderSamples, string export_folder, string export_file){
    m_export_file = export_file;
    m_export_folder = export_folder;
    m_is_rendering = true;
    m_renderSamples = renderSamples;
    m_progress = 0;
}

void Renderer::cancelRender(){
    m_is_rendering = false;
    m_progress = 0;
}