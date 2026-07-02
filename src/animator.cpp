#include "animator.hpp"

vector<float> Animator::getKeyPoses() const {
    vector<float> keyPoses = {};
    if (m_keyFrames.empty()) return keyPoses;
    
    for(const auto& kf : m_keyFrames){
        float pos = kf.keyPos;
        keyPoses.push_back(pos);
    }
    return keyPoses;
}

void Animator::addKeyFrame(KeyFrame keyFrame){
    if (m_numKeyFrames > 0){
        if (m_keyFrames[m_currentKeyFrame].keyPos == m_animationTime)
            return;
    }
    m_currentKeyFrame++;
    m_keyFrames.insert(m_keyFrames.begin() + m_currentKeyFrame, keyFrame);
    m_numKeyFrames++;
}

void Animator::updateCurrentKeyFrame(float animationTime){
    m_animationTime = animationTime;
    m_currentKeyFrame = 0;
    for(int i = 0; i < m_numKeyFrames; i++){
        KeyFrame kf = m_keyFrames[i];
        if (m_animationTime >= kf.keyPos)
            m_currentKeyFrame = std::max(m_currentKeyFrame, i);
    }
    m_currentKeyFrame = glm::clamp(m_currentKeyFrame, 0, m_numKeyFrames - 1);
}

void Animator::previousKeyFrame(){
    if (m_numKeyFrames == 0) return;
    KeyFrame currentkeyFrame = m_keyFrames[m_currentKeyFrame];
    if (m_animationTime <= currentkeyFrame.keyPos)
        m_currentKeyFrame--;
    KeyFrame keyFrame = m_keyFrames[m_currentKeyFrame];
    m_animationTime = keyFrame.keyPos;
}

bool Animator::canGoToPreviousKeyFrame() const { 
    if (m_numKeyFrames == 0) return false;
    KeyFrame currentkeyFrame = m_keyFrames[m_currentKeyFrame];
    return m_currentKeyFrame > 0 || m_animationTime > currentkeyFrame.keyPos; 
}

void Animator::nextKeyFrame(){
    if (m_numKeyFrames == 0) return;
    KeyFrame currentkeyFrame = m_keyFrames[m_currentKeyFrame];
    if (m_animationTime >= currentkeyFrame.keyPos)
        m_currentKeyFrame++;
    KeyFrame keyFrame = m_keyFrames[m_currentKeyFrame];
    m_animationTime = keyFrame.keyPos;
}

bool Animator::canGoToNextKeyFrame() const {
    if (m_numKeyFrames == 0) return false;
    KeyFrame currentkeyFrame = m_keyFrames[m_currentKeyFrame];
    return m_currentKeyFrame < m_numKeyFrames - 1 || m_animationTime < currentkeyFrame.keyPos; 
}

void Animator::animationProcess(){
    if (m_numKeyFrames > 1 && m_prevAnimationTime != m_animationTime){
        KeyFrame prevKf = m_keyFrames[m_currentKeyFrame];
        KeyFrame nextKf = m_keyFrames[std::min(m_currentKeyFrame + 1, m_numKeyFrames - 1)];
        
        float t = 0;
        if (prevKf.keyPos != nextKf.keyPos){
            t = (m_animationTime - prevKf.keyPos) / (nextKf.keyPos - prevKf.keyPos);
            t = glm::clamp(t, 0.0f, 1.0f);
        }
        //ui.modelPos = glm::mix(prevKf.modelPos, nextKf.modelPos, t);
        
        m_prevAnimationTime = m_animationTime;
    }
    
    if (!running()) return;

    if (!m_renderer->isRendering()){
        m_animationFrame++;
        int numFrames = std::floor(m_animationFPS * m_animationDuration);
        if (m_animationFrame >= numFrames) {
            m_is_running = false;
        }
        
        float step = 1 / (float)numFrames;
        m_animationTime += step;
        updateCurrentKeyFrame(m_animationTime);
        
        m_renderer->startRendering(m_renderSamples, to_string(m_animationFrame + 1) + ".png", false);
        m_resetFrame();
    }
    // C:\ffmpeg\bin\ffmpeg.exe -framerate 20 -i %d.png -c:v libx264 -crf 18 -preset slow -pix_fmt yuv420p output.mp4
}

void Animator::start(int renderSamples, int animationFPS, float animationDuration, string export_folder){
    m_animationDuration = animationDuration;
    m_animationFPS = animationFPS;
    m_renderSamples = renderSamples;
    m_export_folder = export_folder;
    m_is_running = true;
    m_animationFrame = 0;
    m_prevAnimationTime = -1;
    m_animationTime = 0;
    updateCurrentKeyFrame(m_animationTime);
    m_renderer->startRendering(m_renderSamples, "1.png", true);
    if (!m_renderer->isRendering()) cancelAnimation(); // Example: dialog canceled
}

void Animator::cancelAnimation(){
    m_is_running = false;
    m_renderer->cancelRender();
}