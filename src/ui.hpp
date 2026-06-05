#include "app.hpp"
#include "shader_program.hpp"
#include <glm/glm.hpp>
#include <functional>
#include "animator.hpp"

using namespace glm;

class UI {
    private:
        bool m_show = false;
        bool m_disabled = false;
        shared_ptr<App> m_app;
        shared_ptr<Renderer> m_renderer;
        shared_ptr<Animator> m_animator;

        void Label(const char* label) const;
        void BeginTwoColumnLayout() const;
        void EndTwoColumnLayout() const;
        void drawMarker(ImVec2 minRect, ImVec2 maxRect, float keyPos) const;
        
        // Materials
        int m_bsdfType = 3;
        float m_roughness = 0;
        float m_metallic = 0;
        float m_metalColor[3] = {1,1,1};
        float m_refractionIndex = 1.33;

        // Technical GPU
        int m_resMultiplier = 3;
        int m_maxBounces = 6;
        int m_bounces = m_maxBounces;

        // Model
        glm::vec3 m_modelPos = glm::vec3(2,-1,-1.5);
        bool m_debugBVH = false;
        bool m_useModel = false;
        
        // Rendering
        int m_renderSamples = 2048;

        // Animation
        int m_animationFPS = 25;
        float m_animationDuration = 1;
        float m_animationTime = 0;
        
    public:
        UI() = default;
        UI(shared_ptr<App> app, shared_ptr<Renderer> renderer, shared_ptr<Animator> animator) 
                                    : m_app(app), m_renderer(renderer), m_animator(animator) { 
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(300, m_app->getIo()->DisplaySize.y));
        };
        
        static bool isInteracting();
        static bool isDragging();
        void toggle();
        void setDisabled(bool disabled) { m_disabled = disabled; }
        void render(function<void()> resetFrame);
        void updateGPU() const;

        int getResolutionMultiplier() const { return m_resMultiplier; }
};