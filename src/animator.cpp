#include "animator.hpp"

int Animator::getClosestKeyFrame() const {
    if (m_numKeyFrames <= 0) return -1;
    KeyFrame prevKf = m_keyFrames[m_currentKeyFrame];
    KeyFrame nextKf = m_keyFrames[std::min(m_currentKeyFrame + 1, m_numKeyFrames - 1)];
    if (abs(prevKf.keyPos - m_animationTime) <= abs(nextKf.keyPos - m_animationTime))
        return m_currentKeyFrame;
    return m_currentKeyFrame + 1;
}

bool Animator::isOnKeyFrame() const {
    if (m_numKeyFrames <= 0) return false;
    return m_keyFrames[getClosestKeyFrame()].keyPos == m_animationTime;
}

vector<float> Animator::getKeyPoses() const {
    vector<float> keyPoses = {};
    if (m_keyFrames.empty()) return keyPoses;
    
    for(const auto& kf : m_keyFrames){
        float pos = kf.keyPos;
        keyPoses.push_back(pos);
    }
    return keyPoses;
}

void Animator::addKeyFrame(){
    KeyFrame keyFrame = {
        m_scene->getState(),
        m_animationTime
    };

    if (m_numKeyFrames > 0){
        int closestKeyFrame = getClosestKeyFrame();
        if (abs(m_keyFrames[closestKeyFrame].keyPos - m_animationTime) < 1e-5f){
            m_keyFrames[closestKeyFrame] = keyFrame;
            return;
        }
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
        if (!running()){
            if (m_animationTime - prevKf.keyPos < 0.05f && m_animationTime - prevKf.keyPos >= 0) 
                m_animationTime = prevKf.keyPos;
            else if (nextKf.keyPos - m_animationTime < 0.05f && nextKf.keyPos - m_animationTime >= 0)
                m_animationTime = nextKf.keyPos;
        }
        
        float t = 0;
        if (prevKf.keyPos != nextKf.keyPos){
            t = (m_animationTime - prevKf.keyPos) / (nextKf.keyPos - prevKf.keyPos);
            t = glm::clamp(t, 0.0f, 1.0f);
        }

        SceneState state = prevKf.state;
        for (int i = 0; i < (int)state.objectStates.size(); i++){
            shared_ptr<Object> correspondingObject = Scene::getObjectFromId(nextKf.state, state.objectStates[i].ID);
            if (correspondingObject == nullptr) continue;

            const Object& interpolatedObj = state.objectStates[i] * (1 - t) + *correspondingObject * t;
            state.objectStates[i] = interpolatedObj;
        }
        m_scene->loadFromState(state, false);
        m_prevAnimationTime = m_animationTime;
    }
    
    if (!running()) return;

    if (!m_renderer->isRendering()){
        m_animationFrame++;
        int numFrames = (int)std::floor(m_animationFPS * m_animationDuration);
        if (m_animationFrame > numFrames) {
            m_is_running = false;
            glfwSetWindowShouldClose(App::Window, true);
            return;
        }
        
        float step = 1 / (float)numFrames;
        m_animationTime += step;
        updateCurrentKeyFrame(m_animationTime);
        
        m_renderer->startRendering(m_renderSamples, to_string(m_animationFrame + 1) + ".png", false, false);
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

void Animator::clear(){
    m_keyFrames.clear();
    m_currentKeyFrame = -1;
    m_numKeyFrames = 0;
    m_animationFrame = 0;
    m_animationTime = 0;
    m_prevAnimationTime = 0;
}