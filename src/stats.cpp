#include "stats.hpp"

void FPSCounter::update(){
    double currentTime = glfwGetTime();
    double deltaTime = currentTime - m_lastTime;
    m_lastTime = currentTime;

    m_frameCount++;
    m_updateTimer += deltaTime;
    if (m_updateTimer >= m_updateInterval && m_frameCount > 0) {
        m_fps = (int)std::round(m_frameCount / m_updateTimer);
        m_updateTimer = 0.0f;
        m_frameCount = 0;
    }
}

void CPUTimer::begin(){
    m_lastTime = glfwGetTime();
}

void CPUTimer::end(){
    double deltaTime = glfwGetTime() - m_lastTime;
    
    m_frameCount++;
    m_updateTimer += deltaTime;
    if (m_updateTimer >= m_updateInterval && m_frameCount > 0) {
        m_time = (float)(m_updateTimer / m_frameCount) * 1000.0f;
        m_updateTimer = 0.0f;
        m_frameCount = 0;
    }
}

void GPUTimer::beginFrame(){
    if (inFlight[writeIndex]){
        GLint available = 0;
        glGetQueryObjectiv(queries[writeIndex], GL_QUERY_RESULT_AVAILABLE, &available);
        if (available){
            GLuint64 elapsedNs;
            glGetQueryObjectui64v(queries[writeIndex], GL_QUERY_RESULT, &elapsedNs);
            lastResultMs = elapsedNs / 1e6;
            inFlight[writeIndex] = false;
        }
    }
    glBeginQuery(GL_TIME_ELAPSED, queries[writeIndex]);
}

void GPUTimer::endFrame(){
    glEndQuery(GL_TIME_ELAPSED);
    inFlight[writeIndex] = true;
    writeIndex = (writeIndex + 1) % NUM_QUERIES;
}