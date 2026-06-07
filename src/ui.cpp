#include "ui.hpp"

void UI::toggle(){
    m_show = !m_show;
}

void UI::drawMarker(ImVec2 minRect, ImVec2 maxRect, float keyPos) const {
    float x = minRect.x + 7 + keyPos * (maxRect.x - minRect.x - 14);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddLine(
        ImVec2(x, minRect.y + 2),
        ImVec2(x, maxRect.y - 2),
        IM_COL32(255, 0, 0, 255),
        2.0f                     
    );
}

void UI::Label(const char* label) const
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("%s", label);
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-FLT_MIN);
}

void UI::BeginTwoColumnLayout() const
{
    ImGui::BeginTable("##layout", 2, ImGuiTableFlags_SizingStretchProp);
    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100);
    ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);
}

void UI::EndTwoColumnLayout() const
{
    ImGui::EndTable();
}

void UI::render(function<void()> resetFrame) {
    if (!m_show) return;

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.5f, 0.2f, 1.0f, 0.3f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.5f, 0.2f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.5f, 0.2f, 1.0f, 0.8f));

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300, m_app->getIo()->DisplaySize.y), ImGuiCond_Always);
    ImGui::Begin("Parameters", (bool*)NULL, ImGuiWindowFlags_MenuBar);

    ImGui::BeginDisabled(m_renderer->isRendering() || m_disabled);

    ImGui::Spacing();
    ImGui::Spacing();

    // ImGui::Spacing();
    // ImGui::Spacing();
    // if (ImGui::CollapsingHeader("Material Settings", ImGuiTreeNodeFlags_DefaultOpen)){
    //     BeginTwoColumnLayout();

    //     const char* items[] = { 
    //         "Lambert", 
    //         "(QON) Qualitative Oren-Nayar", 
    //         "(FON) Fujii-Oren-Nayar", 
    //         "(EON) Energy-Preserving Oren-Nayar"
    //     };
    //     Label("Diffuse Model");
    //     if (ImGui::Combo("##Diffuse Model", &m_bsdfType, items, IM_ARRAYSIZE(items)))
    //         resetFrame();

    //     Label("Ball's Roughness");
    //     if (ImGui::SliderFloat("##Ball's Roughness", &m_roughness, 0, 1))
    //         resetFrame();
            
    //     ImGui::Spacing();
    //     Label("Metal Color");
    //     if (ImGui::ColorEdit3("##Metal Color", m_metalColor))
    //         resetFrame();
    
    //     Label("Metallic");
    //     if (ImGui::SliderFloat("##Metallic", &m_metallic, 0, 1))
    //         resetFrame();

    //     ImGui::Spacing();
    //     Label("Refraction Index");
    //     if (ImGui::SliderFloat("##Refraction Index", &m_refractionIndex, 1, 10))
    //         resetFrame();

    //     EndTwoColumnLayout();
    // }
    if (ImGui::CollapsingHeader("Model Settings", ImGuiTreeNodeFlags_DefaultOpen)){
        BeginTwoColumnLayout();

        Label("Use Model");
        if (ImGui::Checkbox("##Use Model", &m_useModel))
            resetFrame();

        Label("Debug BVH");
        if (ImGui::Checkbox("##Debug BVH", &m_debugBVH))
            resetFrame();

        Label("Model Position");
        if (ImGui::DragFloat3("##Model Position", &m_modelPos[0]))
            resetFrame();

        EndTwoColumnLayout();
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Animation Settings", ImGuiTreeNodeFlags_DefaultOpen)){
       
        BeginTwoColumnLayout();

        Label("Animation Duration");
        ImGui::InputFloat("##Animation Duration", &m_animationDuration);
       
        Label("FPS");
        ImGui::InputInt("##FPS", &m_animationFPS);

        ImGui::Spacing();
        ImGui::Spacing();

        EndTwoColumnLayout();

        ImGui::BeginTable("##layout", 2, ImGuiTableFlags_SizingStretchSame);
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, 0);
        ImGui::BeginDisabled(!m_animator->canGoToPreviousKeyFrame());
        if(ImGui::Button("Previous Key", size)){
            m_animator->previousKeyFrame();
        }
        ImGui::EndDisabled();

        ImGui::TableSetColumnIndex(1);
        ImGui::BeginDisabled(!m_animator->canGoToNextKeyFrame());
        if(ImGui::Button("Next Key", size)){
            m_animator->nextKeyFrame();
        }
        ImGui::EndDisabled();

        EndTwoColumnLayout();

        if(ImGui::Button("Add Keyframe", ImVec2(-FLT_MIN, 0))){
            KeyFrame keyFrame = {
                m_modelPos,
                m_animationTime
            };
            m_animator->addKeyFrame(keyFrame);
        }

        ImGui::Spacing();
        ImGui::SetNextItemWidth(-FLT_MIN);
        m_animationTime = m_animator->getAnimationTime();
        if (ImGui::SliderFloat("##Timeline", &m_animationTime, 0, 1, "")) {
            m_animator->updateCurrentKeyFrame(m_animationTime);
        }
        ImVec2 minRect = ImGui::GetItemRectMin();
        ImVec2 maxRect = ImGui::GetItemRectMax();
        vector<float> keyPoses = m_animator->getKeyPoses();
        for(float pos : keyPoses){
            drawMarker(minRect, maxRect, pos);
        }
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Render Settings", ImGuiTreeNodeFlags_DefaultOpen)){
    
        BeginTwoColumnLayout();
    
        Label("Render Samples");
        ImGui::InputInt("##Render Samples", &m_renderSamples);  
    
        EndTwoColumnLayout();
       
        if (ImGui::Button("Render Image", ImVec2(-FLT_MIN, 0))){
            m_renderer->startRendering(m_renderSamples);
            resetFrame();
        }

        if (ImGui::Button("Render Animation", ImVec2(-FLT_MIN, 0))){
            m_animator->start(m_renderSamples, m_animationFPS, m_animationDuration);
            resetFrame();
        }
       
        if (m_renderer->isRendering()) {
            int progress = m_renderer->getProgress();
            ImGui::ProgressBar((float)progress / m_renderSamples, ImVec2(-FLT_MIN, 0));

            string text = to_string(progress) + " / " + to_string(m_renderSamples);
            float windowWidth = ImGui::GetContentRegionAvail().x;
            float textWidth   = ImGui::CalcTextSize(text.c_str()).x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (windowWidth - textWidth) * 0.5f);
            ImGui::Text(text.c_str());
        }
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Technical Settings", ImGuiTreeNodeFlags_DefaultOpen)){
        BeginTwoColumnLayout();
        
        Label("Max Bounces");
        if (ImGui::InputInt("##Max Bounces", &m_maxBounces)) resetFrame();
        
        Label("Resolution Divider");
        ImGui::InputInt("##Resolution Divider", &m_resMultiplier);
        m_resMultiplier = std::max(m_resMultiplier, 1);
        
        EndTwoColumnLayout();
    }

    ImGui::PopStyleColor(3);
    ImGui::EndDisabled();
    ImGui::End();
}

void UI::updateGPU() const {
    glUniform1i(ShaderProgram::getVarLoc("maxBounces"), m_bounces);
    glUniform1i(ShaderProgram::getVarLoc("useModel"), m_useModel);

    glUniform1i(ShaderProgram::getVarLoc("debugBVH"), m_debugBVH ? 1 : 0);
    glUniform3f(ShaderProgram::getVarLoc("modelPos"), m_modelPos.x, m_modelPos.y, m_modelPos.z);
}

bool UI::isInteracting(){
    return ImGui::IsAnyItemActive() || ImGui::IsWindowFocused();
}

bool UI::isDragging(){
    return ImGui::IsMouseDragging(0) && isInteracting();
}