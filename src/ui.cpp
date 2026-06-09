#include "ui.hpp"

void UI::toggle(){
    m_show = !m_show;
    if (!m_show) m_scene->selectPrimitive(-1);
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

void UI::renderPointer(){
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    ImVec2 pos = ImGui::GetIO().DisplaySize * 0.5;
    draw->AddCircleFilled(pos, 4.0f, IM_COL32(255, 255, 255, 255));
    draw->AddCircleFilled(pos, 2.0f, IM_COL32(0, 0, 0, 255));
}

void UI::renderGizmos(){
    if (m_scene->getSelectedPrimitive() < 0) return;
    Primitive* selectedPrimitive = m_scene->getObject(m_scene->getSelectedPrimitive());

    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    ImU32 orange = IM_COL32(255, 150, 100, 255);

    // Center point
    glm::vec2 primitiveCenter = m_scene->worldToScreen(selectedPrimitive->pos);
    ImVec2 pos = ImGui::GetIO().DisplaySize * 0.5 + ImVec2(primitiveCenter.x, primitiveCenter.y);
    draw->AddCircleFilled(pos, 5, IM_COL32(0, 0, 0, 255));
    draw->AddCircleFilled(pos, 3, orange);

    // // Right arrow
    // draw->AddCircleFilled(pos + ImVec2(13, 0), 3, orange);
    // draw->AddRectFilled(pos + ImVec2(13, -3), pos + ImVec2(50, 3), orange);
    // draw->AddTriangleFilled(pos + ImVec2(60, 0), pos + ImVec2(50, -7), pos + ImVec2(50, 7), orange);
    
    // // Up arrow
    // draw->AddCircleFilled(pos + ImVec2(0, -13), 3, orange);
    // draw->AddRectFilled(pos + ImVec2(-3, -13), pos + ImVec2(3, -50), orange);
    // draw->AddTriangleFilled(pos + ImVec2(0, -60), pos + ImVec2(-7, -50), pos + ImVec2(7, -50), orange);
}

void UI::renderPopupData(Primitive* selectedPrimitive){
    MatType matType = selectedPrimitive->mat.type;
    switch(matType){
        case MatType::DIFFUSE:
            Label("Roughness");
            if (ImGui::SliderFloat("##Roughness", &selectedPrimitive->mat.data.x, 0.0f, 1.0f)){
                selectedPrimitive->mat.data.x = glm::clamp(selectedPrimitive->mat.data.x, 0.0f, 1.0f);
                m_scene->updateScene();
            }
            break;
        case MatType::METAL:
            Label("Fuzziness");
            if (ImGui::SliderFloat("##Fuzziness", &selectedPrimitive->mat.data.x, 0.0f, 0.8f)){
                selectedPrimitive->mat.data.x = glm::clamp(selectedPrimitive->mat.data.x, 0.0f, 0.8f);
                m_scene->updateScene();
            }
            break;
        case MatType::GLASS:
            Label("Refraction Index");
            if (ImGui::SliderFloat("##Refraction Index", &selectedPrimitive->mat.data.y, 1.0f, 4.0f)){
                selectedPrimitive->mat.data.y = glm::clamp(selectedPrimitive->mat.data.y, 1.0f, 4.0f);
                m_scene->updateScene();
            }
            break;
        case MatType::GLOSSY:
            Label("Fuzziness");
            if (ImGui::SliderFloat("##Fuzziness", &selectedPrimitive->mat.data.x, 0.0f, 0.8f)){
                selectedPrimitive->mat.data.x = glm::clamp(selectedPrimitive->mat.data.x, 0.0f, 0.8f);
                m_scene->updateScene();
            }

            Label("Metallic");
            if (ImGui::SliderFloat("##Metallic", &selectedPrimitive->mat.data.y, 0.05f, 1.0f)){
                selectedPrimitive->mat.data.y = glm::clamp(selectedPrimitive->mat.data.y, 0.05f, 1.0f);
                m_scene->updateScene();
            }
            break;
        case MatType::EMIT:
            Label("Intensity");
            if (ImGui::InputFloat("##Intensity", &selectedPrimitive->mat.data.x)){
                selectedPrimitive->mat.data.x = glm::max(selectedPrimitive->mat.data.x, 0.0f);
                m_scene->updateScene();
            }
            break;
        default:
            break;
    }
}

void UI::renderPopup(){
    if (m_scene->getSelectedPrimitive() < 0) return;
    Primitive* selectedPrimitive = m_scene->getObject(m_scene->getSelectedPrimitive());
    
    ImGui::SetNextWindowSizeConstraints(ImVec2(300, 0), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::SeparatorText("Object");
        BeginTwoColumnLayout();
        Label("Position");
        if (ImGui::DragFloat3("##Position", &selectedPrimitive->pos.x, 0.01f))
            m_scene->updateScene();
        
        Label("Scale");
        if (ImGui::DragFloat("##Scale", &selectedPrimitive->scale, 0.001f))
            m_scene->updateScene();
        EndTwoColumnLayout();
        
        ImGui::Spacing();
        ImGui::Spacing();

        ImGui::SeparatorText("Material");
        BeginTwoColumnLayout();
        Label("Color");
        if (ImGui::ColorEdit3("##Color", &selectedPrimitive->mat.color.x))
            m_scene->updateScene();

        const char* items[] = { 
            "Diffuse", 
            "Metal", 
            "Dielectric", 
            "Glossy",
            "Emit"
        };
        Label("Diffuse Model");
        if (ImGui::Combo("##Diffuse Model", (int*)&selectedPrimitive->mat.type, items, IM_ARRAYSIZE(items)))
            m_scene->updateScene();

        renderPopupData(selectedPrimitive);
        EndTwoColumnLayout();
        
        ImGui::Spacing();
        ImGui::Spacing();
        
        ImGui::NewLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0, 0.3, 0.3, 1.0));
        if (ImGui::Button("Remove", ImVec2(-1, 0))){
            m_scene->removeObject(m_scene->getSelectedPrimitive());
            m_scene->selectPrimitive(-1);
        }
        ImGui::PopStyleColor(1);
    }
    ImGui::End();
}

void UI::render() {
    if (!m_show){
        renderPointer();
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.5f, 0.2f, 1.0f, 0.3f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.5f, 0.2f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.5f, 0.2f, 1.0f, 0.8f));
    
    renderPopup();
    renderGizmos();

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300, m_app->getIo()->DisplaySize.y), ImGuiCond_Always);
    ImGui::Begin("Parameters", (bool*)NULL, 0);

    ImGui::BeginDisabled(m_renderer->isRendering() || m_disabled);

    if (ImGui::CollapsingHeader("Camera Settings", ImGuiTreeNodeFlags_DefaultOpen)){
        BeginTwoColumnLayout();

        CameraProperties* camProps = m_camera->getCameraProperties();
        Label("Fov");
        if (ImGui::SliderFloat("##Fov", &camProps->fov, 50.0f, 90.0f))
            m_resetFrame();

        EndTwoColumnLayout();
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Model Settings", ImGuiTreeNodeFlags_DefaultOpen)){
        BeginTwoColumnLayout();

        Label("Use Model");
        if (ImGui::Checkbox("##Use Model", &m_useModel))
            m_resetFrame();

        Label("Debug BVH");
        if (ImGui::Checkbox("##Debug BVH", &m_debugBVH))
            m_resetFrame();

        Label("Model Position");
        if (ImGui::DragFloat3("##Model Position", &m_modelPos[0]))
            m_resetFrame();

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
            m_renderer->startRendering(m_renderSamples, "image.png");
            m_resetFrame();
        }

        if (ImGui::Button("Render Animation", ImVec2(-FLT_MIN, 0))){
            m_animator->start(m_renderSamples, m_animationFPS, m_animationDuration);
            m_resetFrame();
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
        if (ImGui::InputInt("##Max Bounces", &m_maxBounces)) m_resetFrame();
        
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

bool UI::isHovered(){
    return ImGui::GetIO().WantCaptureMouse;
}