#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <functional>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_operation.hpp>
#include <imgui/imgui.h>
#include <imgui/ImGuizmo.h>

#include "../shader_program.hpp"
#include "../app.hpp"
#include "../animator.hpp"
#include "../scene.hpp"
#include "../camera.hpp"
#include "../stats.hpp"

#include "ui_colors.hpp"

using namespace glm;

struct UIContext {
    shared_ptr<App> app;
    shared_ptr<Renderer> renderer;
    shared_ptr<Animator> animator;
    shared_ptr<Scene> scene;
    shared_ptr<Camera> camera;
    shared_ptr<Stats> stats;
    function<void()> resetFrame;
    function<void()> reloadShader;
};

class UI {
    private:
        bool m_show = false;
        bool m_disabled = false;
        UIContext m_context = {};

        void Label(
            const char* label, 
            const string& desc = "", 
            function<void(void)> customWidget = nullptr, 
            float widgetSize = 0.0f
        ) const;

        void BeginTwoColumnLayout() const;
        void EndTwoColumnLayout() const;
        void drawMarker(ImVec2 minRect, ImVec2 maxRect, float keyPos) const;

        bool BeginCustomHeader(const string& name) const;
        void EndCustomHeader() const;
        
        void renderGizmos();

        // Stats
        void renderStats();
        
        bool m_popupOpened = false;
        void renderToolTip(const string& tip) const;
        void renderPopup();
        void renderPopupData(Object* selectedObject);
        void renderParameters();
        int renderListItems(const vector<const char*>& items, int* selectedIndex);
        void renderScene();
        float m_windowSplit = 200.0f;

        void importModelDialog();
        void importFurDialog();
        void renderMainMenuBar();

        void renderPointer();

        // Technical GPU
        int m_resMultiplier = 3;
        int m_maxBounces = 10;
        
        // Camera
        float m_fov = 50.0f;
        
        // Model
        bool m_debugBVH = false;
        
        // Rendering
        int m_renderSamples = 2048;
        
        // Animation
        int m_animationFPS = 25;
        float m_animationDuration = 1.0f;
        float m_animationTime = 0.0f;
        
    public:
        UI() = default;
        UI(UIContext context)
            : m_context(std::move(context)) 
        { 
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(300.0f, ImGui::GetIO().DisplaySize.y));
            ImGui::StyleColorsDark(nullptr);
        };
            
        static bool isInteracting();
        static bool isDragging();
        static bool isHovered();
        bool isShowing() const { return m_show; }
        void toggle();
        void setDisabled(bool disabled) { m_disabled = disabled; }
        void render();
        void updateGPU() const;
        
        enum TexType : int {
            Result = 0,
            Color = 1,
            Albedo = 2,
            Normal = 3,
            Depth = 4,
            Selection = 5,
            Denoised = 6,
        };
        int textureDisplay = TexType::Result;
        int getResolutionMultiplier() const { return m_resMultiplier; }
};