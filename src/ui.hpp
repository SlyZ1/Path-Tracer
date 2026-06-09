#define IMGUI_DEFINE_MATH_OPERATORS
#include "app.hpp"
#include "shader_program.hpp"
#include <glm/glm.hpp>
#include <functional>
#include "animator.hpp"
#include "scene.hpp"
#include "camera.hpp"

using namespace glm;

class UI {
    private:
        bool m_show = false;
        bool m_disabled = false;
        shared_ptr<App> m_app;
        shared_ptr<Renderer> m_renderer;
        shared_ptr<Animator> m_animator;
        shared_ptr<Scene> m_scene;
        shared_ptr<Camera> m_camera;
        function<void()> m_resetFrame;

        void Label(const char* label) const;
        void BeginTwoColumnLayout() const;
        void EndTwoColumnLayout() const;
        void drawMarker(ImVec2 minRect, ImVec2 maxRect, float keyPos) const;

        void renderGizmos();

        bool m_popupOpened = false;
        void renderPopup();
        void renderPopupData(Primitive* selectedPrimitive);

        void renderPointer();

        // Technical GPU
        int m_resMultiplier = 3;
        int m_maxBounces = 6;
        int m_bounces = m_maxBounces;

        // Camera
        float m_fov = 50.0f;

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
        UI(shared_ptr<App> app, shared_ptr<Renderer> renderer, shared_ptr<Animator> animator, shared_ptr<Scene> scene, 
            shared_ptr<Camera> camera, function<void()> resetFrame)
                : m_app(app), m_renderer(renderer), m_animator(animator), m_scene(scene), m_camera(camera), 
                m_resetFrame(resetFrame) { 
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(300, m_app->getIo()->DisplaySize.y));
        };
        
        static bool isInteracting();
        static bool isDragging();
        static bool isHovered();
        bool isShowing() const { return m_show; }
        void toggle();
        void setDisabled(bool disabled) { m_disabled = disabled; }
        void render();
        void updateGPU() const;

        int getResolutionMultiplier() const { return m_resMultiplier; }
};