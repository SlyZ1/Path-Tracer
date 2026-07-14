#include "ui.hpp"

void UI::toggle(){
    m_show = !m_show;
    if (!m_show) m_scene->selectObject(-1);
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

void UI::renderToolTip(const string& tip) const {
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(300.0f);
        ImGui::TextUnformatted(tip.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void TextRight(const char* text){
    float textWidth = ImGui::CalcTextSize(text).x;
    float availWidth = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, availWidth - textWidth));
    ImGui::Text("%s", text);
}

void UI::Label(const char* label, const string& desc) const
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("%s", label);
    if (!desc.empty())
        renderToolTip(desc);
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-FLT_MIN);
}

void UI::BeginTwoColumnLayout() const
{
    float availWidth = ImGui::GetContentRegionAvail().x;
    float labelWidth = std::max(availWidth * 0.4f, 120.0f);
    ImGui::BeginTable("##layout", 2, ImGuiTableFlags_SizingStretchProp);
    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
    ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);
}

void UI::EndTwoColumnLayout() const
{
    ImGui::EndTable();
}

bool UI::BeginCustomHeader(const string& name) const {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    ImVec2 padding = ImGui::GetStyle().WindowPadding;
    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
    ImGui::BeginChild((name + "_group").c_str(), ImVec2(-FLT_MIN, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding);
    bool open = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth);
    if (open){
        ImGui::Spacing();
        ImGui::Spacing();
    }
    return open;
}

void UI::EndCustomHeader() const {
    ImGui::EndChild();
    ImGui::PopStyleVar(1);
    ImGui::PopStyleColor(1);
}

void UI::renderPointer(){
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    ImVec2 pos = ImGui::GetIO().DisplaySize * 0.5;
    draw->AddCircleFilled(pos, 4.0f, IM_COL32(255, 255, 255, 255));
    draw->AddCircleFilled(pos, 2.0f, IM_COL32(0, 0, 0, 255));
}

void UI::renderGizmos(){
    if (m_scene->getSelectedObject() < 0) return;
    Object* selectedObject = m_scene->getObject(m_scene->getSelectedObject());

    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    ImU32 orange = IM_COL32(255, 150, 100, 255);

    // Center point
    glm::vec2 objectCenter = m_scene->worldToScreen(selectedObject->pos);
    ImVec2 pos = ImGui::GetIO().DisplaySize * 0.5 + ImVec2(objectCenter.x, objectCenter.y);
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

void UI::renderStats(int frameAccumulator){
    float currentTime = (float)glfwGetTime();
    float deltaTime = currentTime - m_lastTime;
    m_lastTime = currentTime;

    m_fpsFrameCount++;
    m_fpsUpdateTimer += deltaTime;
    constexpr float fpsUpdateInterval = 0.25f;
    if (m_fpsUpdateTimer >= fpsUpdateInterval && m_fpsFrameCount > 0) {
        m_displayedFps = (int)round(m_fpsFrameCount / m_fpsUpdateTimer);
        m_displayedFrameTimeMs = (m_fpsUpdateTimer / m_fpsFrameCount) * 1000.0f;
        m_fpsUpdateTimer = 0.0f;
        m_fpsFrameCount = 0;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags flags =  ImGuiWindowFlags_AlwaysAutoResize
                            | ImGuiWindowFlags_NoCollapse
                            | ImGuiWindowFlags_NoDecoration
                            | ImGuiWindowFlags_NoMove
                            | ImGuiWindowFlags_NoMouseInputs
                            | ImGuiWindowFlags_NoResize;
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x, ImGui::GetMainViewport()->WorkPos.y), ImGuiCond_Always, ImVec2(1, 0));
    if (ImGui::Begin("Stats", nullptr, flags)) {
        BeginTwoColumnLayout();

        Label("Accumulation:");
        ImGui::Text("%d", frameAccumulator);

        Label("FPS:");
        ImGui::Text("%d", m_displayedFps);

        Label("Frame Time:");
        ImGui::Text("%.2fms", m_displayedFrameTimeMs);

        EndTwoColumnLayout();
    }
    ImGui::End();
}

void UI::renderPopupData(Object* selectedObject){
    ImGui::Spacing();
    ImGui::Spacing();
    MatType matType = selectedObject->mat.type;
    switch(matType){
        case MatType::DIFFUSE:
            Label("Roughness", "Oren-Nayar's roughness for diffuse materials.\nUsed in the Energy-Preserving Oren-Nayar's model (EON).");
            selectedObject->mat.data.x = glm::clamp(selectedObject->mat.data.x, 0.0f, 1.0f);
            if (ImGui::SliderFloat("##Roughness", &selectedObject->mat.data.x, 0.0f, 1.0f)){
                selectedObject->mat.data.x = glm::clamp(selectedObject->mat.data.x, 0.0f, 1.0f);
                m_scene->updateSceneNextFrame();
            }
            break;
        case MatType::METAL:
            Label("Fuzziness", "Controls how unpolished the metal will be.\nUsed in the Cook-Torrance model with the GGX microfacets distribution.");
            selectedObject->mat.data.x = glm::clamp(selectedObject->mat.data.x, 0.0f, 0.8f);
            if (ImGui::SliderFloat("##Fuzziness", &selectedObject->mat.data.x, 0.0f, 0.8f)){
                selectedObject->mat.data.x = glm::clamp(selectedObject->mat.data.x, 0.0f, 0.8f);
                m_scene->updateSceneNextFrame();
            }
            if (m_scene->getSpectral()){
                Label("Film IOR", "Index of refraction of the thin film on the surface.");
                selectedObject->mat.data2.x = glm::clamp(selectedObject->mat.data2.x, 1.0f, 2.0f);
                if (ImGui::SliderFloat("##IOR", &selectedObject->mat.data2.x, 1.0f, 2.0f)){
                    selectedObject->mat.data2.x = glm::clamp(selectedObject->mat.data2.x, 1.0f, 2.0f);
                    m_scene->updateSceneNextFrame();
                }
                Label("Film Depth", "Depth of the thin film on the surface.");
                selectedObject->mat.data2.y = glm::clamp(selectedObject->mat.data2.y, 0.0f, 1000.0f);
                if (ImGui::SliderFloat("##Film Depth", &selectedObject->mat.data2.y, 0.0f, 1000.0f)){
                    selectedObject->mat.data2.y = glm::clamp(selectedObject->mat.data2.y, 0.0f, 1000.0f);
                    m_scene->updateSceneNextFrame();
                }
            }
            break;
        case MatType::GLASS:
            Label("Fuzziness", "Controls how unpolished the glass will be.\nUsed in the Cook-Torrance model with the GGX microfacets distribution.");
            selectedObject->mat.data.x = glm::clamp(selectedObject->mat.data.x, 0.0f, 0.5f);
            if (ImGui::SliderFloat("##Fuzziness", &selectedObject->mat.data.x, 0.0f, 0.5f)){
                selectedObject->mat.data.x = glm::clamp(selectedObject->mat.data.x, 0.0f, 0.5f);
                m_scene->updateSceneNextFrame();
            }
            Label("IOR", "Index of refraction of the dielectric.");
            selectedObject->mat.data.y = glm::clamp(selectedObject->mat.data.y, 1.0f, 2.0f);
            if (ImGui::SliderFloat("##IOR", &selectedObject->mat.data.y, 1.0f, 2.0f)){
                selectedObject->mat.data.y = glm::clamp(selectedObject->mat.data.y, 1.0f, 2.0f);
                m_scene->updateSceneNextFrame();
            }
            if (m_scene->getSpectral()){
                Label("Dispertion Factor", "How much the IOR varies based on wavelength, using Cauchy's approximation.");
                selectedObject->mat.data2.x = glm::clamp(selectedObject->mat.data2.x, 0.0f, 0.05f);
                if (ImGui::SliderFloat("##Dispertion Factor", &selectedObject->mat.data2.x, 0.0f, 0.05f)){
                    selectedObject->mat.data2.x = glm::clamp(selectedObject->mat.data2.x, 0.0f, 0.05f);
                    m_scene->updateSceneNextFrame();
                }
            }
            ImGui::Spacing();
            ImGui::Spacing();
            Label("Absorption Factor", "How much light is absorbed in the dielectric, using Beer-Lambert's law.");
            selectedObject->mat.data.z = glm::clamp(selectedObject->mat.data.z, 0.0f, 2.0f);
            if (ImGui::SliderFloat("##Absorption Factor", &selectedObject->mat.data.z, 0.0f, 2.0f)){
                selectedObject->mat.data.z = glm::clamp(selectedObject->mat.data.z, 0.0f, 2.0f);
                m_scene->updateSceneNextFrame();
            }
            Label("Scattering Factor", "How much light is scattered in the dielectric.");
            selectedObject->mat.data.w = glm::clamp(selectedObject->mat.data.w, 0.0f, 2.0f);
            if (ImGui::SliderFloat("##Scattering Factor", &selectedObject->mat.data.w, 0.0f, 2.0f)){
                selectedObject->mat.data.w = glm::clamp(selectedObject->mat.data.w, 0.0f, 2.0f);
                m_scene->updateSceneNextFrame();
            }
            Label("Anisotropy", "How much the ray's direction is taken into account when scattering.\ng=1: forward scattering\ng=0: isotropic scattering\ng=-1: backward scattering");
            selectedObject->mat.data2.y = glm::clamp(selectedObject->mat.data2.y, -1.0f, 1.0f);
            if (ImGui::SliderFloat("##Anisotropy", &selectedObject->mat.data2.y, -1.0f, 1.0f)){
                selectedObject->mat.data2.y = glm::clamp(selectedObject->mat.data2.y, -1.0f, 1.0f);
                m_scene->updateSceneNextFrame();
            }
            break; 
        case MatType::GLOSSY:
            Label("Fuzziness", "Controls how unpolished the object will be.\nUsed in the Cook-Torrance model with the GGX microfacets distribution.");
            selectedObject->mat.data.x = glm::clamp(selectedObject->mat.data.x, 0.0f, 0.8f);
            if (ImGui::SliderFloat("##Fuzziness", &selectedObject->mat.data.x, 0.0f, 0.8f)){
                selectedObject->mat.data.x = glm::clamp(selectedObject->mat.data.x, 0.0f, 0.8f);
                m_scene->updateSceneNextFrame();
            }
            Label("Metallic");
            selectedObject->mat.data.y = glm::clamp(selectedObject->mat.data.y, 0.05f, 1.0f);
            if (ImGui::SliderFloat("##Metallic", &selectedObject->mat.data.y, 0.05f, 1.0f)){
                selectedObject->mat.data.y = glm::clamp(selectedObject->mat.data.y, 0.05f, 1.0f);
                m_scene->updateSceneNextFrame();
            }
            break;
        case MatType::EMIT:
            Label("Intensity");
            selectedObject->mat.data.x = glm::max(selectedObject->mat.data.x, 0.0f);
            if (ImGui::DragFloat("##Intensity", &selectedObject->mat.data.x)){
                selectedObject->mat.data.x = glm::max(selectedObject->mat.data.x, 0.0f);
                m_scene->updateSceneNextFrame();
            }
            break;
        default:
            break;
    }
}

void UI::renderPopup(){
    if (m_scene->getSelectedObject() < 0) return;
    Object* selectedObject = m_scene->getObject(m_scene->getSelectedObject());
    
    ImGui::SetNextWindowSizeConstraints(ImVec2(300, 0), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::SeparatorText("Object");
        BeginTwoColumnLayout();
        Label("Position");
        if (ImGui::DragFloat3("##Position", &selectedObject->pos.x, 0.01f))
            m_scene->updateSceneNextFrame();
        
        Label("Rotation");
        selectedObject->rotation = mod(mod(selectedObject->rotation, 360.0f) + 360.0f, 360.0f);
        if (ImGui::DragFloat3("##Rotation", &selectedObject->rotation.x, 0.1f)){
            selectedObject->rotation = mod(mod(selectedObject->rotation, 360.0f) + 360.0f, 360.0f);
            m_scene->updateSceneNextFrame();
        }
        Label("Scale");
        if (selectedObject->type == PrimType::SPHERE && selectedObject->mat.type == MatType::EMIT){

            selectedObject->scale = max(vec3(selectedObject->scale.x), vec3(0.0f));
            if (ImGui::DragFloat("##Scale", &selectedObject->scale.x, 0.001f)){
                selectedObject->scale = max(vec3(selectedObject->scale.x), vec3(0.0f));
                m_scene->updateSceneNextFrame();
            }
        }
        else{
            selectedObject->scale = max(selectedObject->scale, vec3(0.0f));
            if (ImGui::DragFloat3("##Scale", &selectedObject->scale.x, 0.001f)){
                selectedObject->scale = max(selectedObject->scale, vec3(0.0f));
                m_scene->updateSceneNextFrame();
            }
        }

        ImGui::Spacing();
        ImGui::Spacing();
        
        Label("Shape Type");
        if (ImGui::Combo("##Shape Type", (int*)(&selectedObject->type), Scene::primLabels, IM_ARRAYSIZE(Scene::primLabels))){
            m_scene->updateSceneNextFrame();
        }

        if (selectedObject->type == PrimType::MESH_){
            Label("Mesh Used");
            vector<const char*> meshes = m_scene->getMeshNames();
            if (ImGui::Combo("##Model", &selectedObject->meshIndex, meshes.data(), (int)meshes.size())){
                m_scene->updateSceneNextFrame();
            }

            Label("Is Smooth");
            if (ImGui::Checkbox("##Is Smooth", &selectedObject->isSmooth))
                m_scene->updateSceneNextFrame();
        }
        EndTwoColumnLayout();
        
        ImGui::Spacing();
        ImGui::Spacing();

        ImGui::SeparatorText("Material");
        BeginTwoColumnLayout();
        Label("Color");
        if (ImGui::ColorEdit3("##Color", &selectedObject->mat.color.x))
            m_scene->updateSceneNextFrame();

        const char* items[] = { 
            "Diffuse", 
            "Metal", 
            "Dielectric", 
            "Glossy",
            "Emit"
        };
        Label("Material type");
        if (ImGui::Combo("##Material type", (int*)&selectedObject->mat.type, items, IM_ARRAYSIZE(items)))
            m_scene->updateSceneNextFrame();

        renderPopupData(selectedObject);
        EndTwoColumnLayout();
        
        ImGui::Spacing();
        ImGui::Spacing();
        
        ImGui::NewLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.3f, 0.3f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 0.8f));
        if (ImGui::Button("Remove", ImVec2(-1, 0))){
            m_scene->removeObject(m_scene->getSelectedObject());
        }
        ImGui::PopStyleColor(3);
    }
    ImGui::End();
}

void UI::renderScene(){
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 3.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(-FLT_MIN, 250.0f), ImVec2(-FLT_MIN, ImGui::GetIO().DisplaySize.y - 250.0f));
    ImGui::BeginChild("SceneContainer", ImVec2(-FLT_MIN, 200.0f), ImGuiChildFlags_ResizeY | ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(1);

    ImGui::BeginChild("Scene", ImVec2(-FLT_MIN, -FLT_MIN), ImGuiChildFlags_Borders);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 4.0f));

    ImGui::PopStyleVar(1);
    ImGui::EndChild();
    ImGui::EndChild();
}

void UI::renderParameters(){
    ImGui::BeginChild("Parameters", ImVec2(-FLT_MIN, -FLT_MIN), ImGuiChildFlags_Borders);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 4.0f));
    ImGui::BeginDisabled(m_renderer->isRendering() || m_disabled);

    float padding = ImGui::GetStyle().WindowPadding.y;
    float buttonHeight = 40;

    
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 0.0f);
    ImGui::BeginChild("Scrollable Parameters", ImVec2(-FLT_MIN, -(buttonHeight + padding)), ImGuiChildFlags_None, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    if (BeginCustomHeader("Camera Settings")){
        BeginTwoColumnLayout();
        CameraProperties* camProps = m_camera->getCameraProperties();

        Label("Fov");
        if (ImGui::SliderFloat("##Fov", &camProps->fov, 20.0f, 90.0f))
            m_resetFrame();
        
        Label("Aperture");
        if (ImGui::SliderFloat("##Aperture", &camProps->aperture, 0.0f, 1.0f))
            m_resetFrame();
        
        Label("Focal Length");
        if (ImGui::DragFloat("##Focal Length", &camProps->focalLength, 0.1f, -1.0f, FLT_MAX))
            m_resetFrame();

        EndTwoColumnLayout();
        ImGui::TreePop();
    }
    EndCustomHeader();
    
    if (BeginCustomHeader("Sky Box Settings")){
        BeginTwoColumnLayout();

        Label("Sky Intensity");
        if (ImGui::SliderFloat("##Sky Intensity", &m_skyIntensity, 0.0f, 1.0f))
            m_resetFrame();
        
        Label("Sky Top Color");
        if (ImGui::ColorEdit3("##Sky Top Color", &m_skyTopColor.x))
            m_resetFrame();
            
        Label("Sky Middle Color");
        if (ImGui::ColorEdit3("##Sky Middle Color", &m_skyMiddleColor.x))
            m_resetFrame();
            
        Label("Sky Bottom Color");
        if (ImGui::ColorEdit3("##Sky Bottom Color", &m_skyBottomColor.x))
            m_resetFrame();

        EndTwoColumnLayout();
        ImGui::TreePop();
    }
    EndCustomHeader();

    if (BeginCustomHeader("Model Settings")){
        BeginTwoColumnLayout();

        Label("Debug BVH");
        if (ImGui::Checkbox("##Debug BVH", &m_debugBVH))
            m_resetFrame();

        EndTwoColumnLayout();
        ImGui::TreePop();
    }
    EndCustomHeader();

    if (BeginCustomHeader("Animation Settings")){
       
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

        string keyFrameLabel = "Add Keyframe";
        if (m_animator->isOnKeyFrame()) keyFrameLabel = "Update Keyframe";
        if(ImGui::Button(keyFrameLabel.c_str(), ImVec2(-FLT_MIN, 0))){
            m_animator->addKeyFrame();
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
        ImGui::TreePop();
    }
    EndCustomHeader();

    if (BeginCustomHeader("Render Settings")){
    
        BeginTwoColumnLayout();
    
        Label("Render Samples");
        ImGui::InputInt("##Render Samples", &m_renderSamples);  
    
        EndTwoColumnLayout();
       
        if (ImGui::Button("Export Textures", ImVec2(-FLT_MIN, 0))){
            m_renderer->startRendering(m_renderSamples, "result.png", true, true);
        }

        if (ImGui::Button("Render Image", ImVec2(-FLT_MIN, 0))){
            m_renderer->startRendering(m_renderSamples);
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
            ImGui::Text("%s", text.c_str());
        }
        ImGui::TreePop();
    }
    EndCustomHeader();

    if (BeginCustomHeader("Technical Settings")){
        BeginTwoColumnLayout();
        
        Label("Max Bounces");
        if (ImGui::InputInt("##Max Bounces", &m_maxBounces)) m_resetFrame();
        
        Label("Resolution Divider");
        ImGui::InputInt("##Resolution Divider", &m_resMultiplier);
        m_resMultiplier = std::max(m_resMultiplier, 1);

        Label("Texture Display");
        static const char* textureTypes[]{"Result", "Color", "Albedo", "Normal", "Depth", "Denoised"};
        ImGui::Combo("##Texture Display", &textureDisplay, textureTypes, IM_ARRAYSIZE(textureTypes));
        
        EndTwoColumnLayout();
        ImGui::TreePop();
    }
    EndCustomHeader();

    ImGui::EndChild(); 
    ImGui::PopStyleVar(2);
    
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - buttonHeight - padding);
    bool isSpectral = m_scene->getSpectral();
    string spectralButtonLabel = "Toggle Spectral : " + string(isSpectral ? "ON" : "OFF");
    if (ImGui::Button(spectralButtonLabel.c_str(), ImVec2(-FLT_MIN, buttonHeight))){
        m_scene->setSpectral(!isSpectral);
        m_reloadShader();
    }

    ImGui::EndDisabled();
    ImGui::PopStyleVar(1);
    ImGui::EndChild();
}

void UI::render(int frameAccumulator) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Import Model (.obj)"))  {
                bool cancel;
                nfdopendialogu8args_t args = {0};
                nfdfilteritem_t filters[] = {
                    { "3D Models", "obj" },
                    { "OBJ", "obj" }
                };
                args.filterList = filters;
                args.filterCount = 2;
                string path = m_app->openFileDialog(cancel, args);
                shared_ptr<Mesh> newMesh = make_shared<Mesh>();
                newMesh->loadFromModel(path.c_str());
                m_scene->addMesh(newMesh);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit"))  {
                glfwSetWindowShouldClose(m_app->getWindow(), true);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Scene")) {
            if (ImGui::MenuItem("Save")) {
                bool cancel;
                string path = m_app->saveFileDialog(cancel, "scene.json");
                m_scene->stateToJson(m_scene->getState(), path);
            }
            if (ImGui::MenuItem("Load"))  {
                bool cancel;
                nfdopendialogu8args_t args = {0};
                nfdfilteritem_t filters[] = {
                    { "JSON", "json" },
                    { "json" }
                };
                args.filterList = filters;
                args.filterCount = 1;
                string path = m_app->openFileDialog(cancel, args);
                m_scene->loadFromState(m_scene->stateFromJson(path));
                m_animator->clear();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    float menuBarHeight = ImGui::GetMainViewport()->WorkPos.y;
    float workingFrameHeight = ImGui::GetMainViewport()->WorkSize.y;

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
        
            renderStats(frameAccumulator);
            if (!m_show){
                renderPointer();
                ImGui::PopStyleColor(4);
                ImGui::PopStyleVar(2);
                return;
            }
            
            renderPopup();
            renderGizmos();
        
        ImGui::PopStyleColor(1);
        ImGui::PopStyleVar(1);

        
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.1f, 0.1f, 0.1f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.1f, 0.1f, 0.1f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 2.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 2.0f));
        
            ImGui::SetNextWindowPos(ImVec2(0, menuBarHeight), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(400, workingFrameHeight), ImGuiCond_Once);
            ImGui::SetNextWindowSizeConstraints(ImVec2(300, workingFrameHeight), 
                                                ImVec2(ImGui::GetIO().DisplaySize.x / 2.0f, workingFrameHeight));
            ImGui::Begin("Left Window", (bool*)NULL, ImGuiWindowFlags_NoCollapse 
                                                    | ImGuiWindowFlags_NoTitleBar 
                                                    | ImGuiWindowFlags_NoBringToFrontOnFocus);
                                                    
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
                renderScene();
                renderParameters();
            ImGui::PopStyleVar(1);

            ImGui::End();
        
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(4);

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(1);
}

void UI::updateGPU() const {
    glUniform1i(ShaderProgram::getVarLoc("maxBounces"), m_maxBounces);

    glUniform1i(ShaderProgram::getVarLoc("debugBVH"), m_debugBVH ? 1 : 0);

    glUniform1f(ShaderProgram::getVarLoc("skyIntensity"), m_skyIntensity);
    vec3 skyBottomColor = m_skyBottomColor;
    vec3 skyMiddleColor = m_skyMiddleColor;
    vec3 skyTopColor = m_skyTopColor;
    if (m_scene->getSpectral()){
        skyBottomColor = Material::rgbToSigmoidCoeffs(skyBottomColor);
        skyMiddleColor = Material::rgbToSigmoidCoeffs(skyMiddleColor);
        skyTopColor = Material::rgbToSigmoidCoeffs(skyTopColor);
    }
    glUniform3f(ShaderProgram::getVarLoc("skyBottomColor"), skyBottomColor.x, skyBottomColor.y, skyBottomColor.z);
    glUniform3f(ShaderProgram::getVarLoc("skyMiddleColor"), skyMiddleColor.x, skyMiddleColor.y, skyMiddleColor.z);
    glUniform3f(ShaderProgram::getVarLoc("skyTopColor"), skyTopColor.x, skyTopColor.y, skyTopColor.z);
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