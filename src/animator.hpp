#ifndef ANIMATOR
#define ANIMATOR

#include "renderer.hpp"
#include <functional>
#include <glm/glm.hpp>
#include "scene.hpp"

struct KeyFrame {
    SceneState state;
    float keyPos;
};

class Animator {
private:
    bool m_is_running = false;
    shared_ptr<Renderer> m_renderer;
    shared_ptr<Scene> m_scene;

    int m_currentKeyFrame = -1;
    int m_numKeyFrames = 0;
    int m_animationFPS = 25;
    int m_animationFrame = 0;
    float m_animationDuration = 1;
    float m_animationTime = 0;
    float m_prevAnimationTime = 0;
    vector<KeyFrame> m_keyFrames = {};

    string m_export_folder = {};
    int m_renderSamples;
    function<void()> m_resetFrame;

    int getClosestKeyFrame() const;
    
public: 
    Animator() = default;
    Animator(shared_ptr<Renderer> renderer, shared_ptr<Scene> scene, function<void()> resetFrame) 
    : m_renderer(renderer), m_scene(scene), m_resetFrame(resetFrame) {}

    vector<float> getKeyPoses() const;
    bool running() const { return m_is_running; }
    bool isOnKeyFrame() const;
    float getAnimationTime() const { return m_animationTime; }
    void addKeyFrame();
    void updateCurrentKeyFrame(float animationTime);
    void previousKeyFrame();
    bool canGoToPreviousKeyFrame() const;
    void nextKeyFrame();
    bool canGoToNextKeyFrame() const;
    void start(int renderSamples, int animationFPS, float animationDuration, string export_folder = "", bool renderTextures = false);
    void animationProcess();
    void cancelAnimation();
    void clear();
};

#endif