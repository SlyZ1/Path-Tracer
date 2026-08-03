#include "ui.hpp"

void UI::toggle(){
    m_show = !m_show;
    if (!m_show) m_context.scene->selectObject(-1);
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

void TextWithShadow(const char* text, ImVec4 textColor = ImVec4(1,1,1,1), ImVec4 shadowColor = ImVec4(0,0,0,0.6f), ImVec2 offset = ImVec2(1,1)){
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float rowHeight = ImGui::GetTextLineHeightWithSpacing();
    float textHeight = ImGui::GetTextLineHeight();

    pos.y += (rowHeight - textHeight) * 0.5f; 
    ImDrawList* draw = ImGui::GetWindowDrawList();
    
    draw->AddText(ImVec2(pos.x + offset.x, pos.y + offset.y), ImGui::GetColorU32(shadowColor), text);
    draw->AddText(pos, ImGui::GetColorU32(textColor), text);
    
    ImVec2 textSize = ImGui::CalcTextSize(text);
    ImGui::Dummy(textSize);
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

void UI::Label(const char* label, const string& desc, function<void(void)> customWidget, float widgetSize) const
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    TextWithShadow(label, ImVec4(1,1,1,1), ImVec4(0.05f,0.05f,0.05f,0.7f), ImVec2(1,1));
    if (!desc.empty())
        renderToolTip(desc); 
    if (customWidget != nullptr){
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - widgetSize);
        customWidget();
    }
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
    ImGui::PushStyleColor(ImGuiCol_ChildBg, UIColors::fgColor);
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
    if (m_disabled) return;
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    ImVec2 pos = ImGui::GetIO().DisplaySize * 0.5;
    draw->AddCircleFilled(pos, 4.0f, IM_COL32(255, 255, 255, 255));
    draw->AddCircleFilled(pos, 2.0f, IM_COL32(0, 0, 0, 255));
}

void UI::renderGizmos(){
    ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
    mat4 view = m_context.camera->viewMatrix();
    mat4 projection = Intersections::projectionMatrix(m_context.app, m_context.camera);

    mat4 axesView = glm::lookAt(vec3(0.0f), m_context.camera->lookDir(), vec3(0,1,0));
    mat4 axesModel = mat4(1.0f);
    ImGuizmo::RecomposeMatrixFromComponents(
        &(m_context.camera->lookDir() * 3.0f / radians(m_context.camera->getCameraProperties()->fov))[0],
        &vec3(0.0f)[0], 
        &vec3(1.0f)[0], 
        &axesModel[0][0]
    );
    float width = (float)m_context.app->width();
    float height = (float)m_context.app->height();
    ImGuizmo::SetRect(width*0.95f - 100.0f, height*0.96f - 100.0f, width*0.1f, height*0.1f);
    ImGuizmo::DrawAxes(&axesView[0][0], &projection[0][0], &axesModel[0][0], 1);
    ImGuizmo::SetRect(0, 0, width, height);

    if (m_context.scene->getSelectedObject() < 0) return;
    Object* selectedObject = m_context.scene->getObject(m_context.scene->getSelectedObject());
    if (selectedObject == nullptr) return;
    
    mat4 model = mat4(1.0f);
    ImGuizmo::RecomposeMatrixFromComponents(
        &selectedObject->pos.x, 
        &selectedObject->rotation.x, 
        &selectedObject->scale.x, 
        &model[0][0]
    );

    ImGuizmo::Enable(!m_disabled);
    ImGuizmo::Manipulate(
        &view[0][0], 
        &projection[0][0], 
        ImGuizmo::OPERATION::TRANSLATE | ImGuizmo::OPERATION::SCALE | ImGuizmo::OPERATION::ROTATE,  
        ImGuizmo::MODE::LOCAL,
        &model[0][0],
        nullptr
    );

    vec3 pos, rotation, scale;
    ImGuizmo::DecomposeMatrixToComponents(&model[0][0], &pos.x, &rotation.x, &scale.x);

    selectedObject->pos = pos;
    selectedObject->scale = scale;
    selectedObject->rotation = rotation;

    if (ImGuizmo::IsUsingAny()){
        m_context.resetFrame();
        m_context.scene->updateScene();
    }
}

void UI::renderStats(){
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
        ImGui::Text("%d", m_context.stats->numFrames);

        Label("FPS:");
        ImGui::Text("%d", m_context.stats->fps);

        Label("Frame Time:");
        ImGui::Text("%.2fms", m_context.stats->frameTime);

        Label("GPU Time:");
        ImGui::Text("%.2fms", m_context.stats->GPUTime);

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
                m_context.scene->updateSceneNextFrame();
            }
            break;
        case MatType::METAL:
            Label("Fuzziness", "Controls how unpolished the metal will be.\nUsed in the Cook-Torrance model with the GGX microfacets distribution.");
            selectedObject->mat.data.x = glm::clamp(selectedObject->mat.data.x, 0.0f, 0.8f);
            if (ImGui::SliderFloat("##Fuzziness", &selectedObject->mat.data.x, 0.0f, 0.8f)){
                selectedObject->mat.data.x = glm::clamp(selectedObject->mat.data.x, 0.0f, 0.8f);
                m_context.scene->updateSceneNextFrame();
            }
            if (m_context.scene->getSpectral()){
                Label("Film IOR", "Index of refraction of the thin film on the surface.");
                selectedObject->mat.data2.z = glm::clamp(selectedObject->mat.data2.z, 1.0f, 2.0f);
                if (ImGui::SliderFloat("##Film IOR", &selectedObject->mat.data2.z, 1.0f, 2.0f)){
                    selectedObject->mat.data2.z = glm::clamp(selectedObject->mat.data2.z, 1.0f, 2.0f);
                    m_context.scene->updateSceneNextFrame();
                }
                Label("Film Depth", "Depth of the thin film on the surface.");
                selectedObject->mat.data2.w = glm::clamp(selectedObject->mat.data2.w, 0.0f, 1000.0f);
                if (ImGui::SliderFloat("##Film Depth", &selectedObject->mat.data2.w, 0.0f, 1000.0f)){
                    selectedObject->mat.data2.w = glm::clamp(selectedObject->mat.data2.w, 0.0f, 1000.0f);
                    m_context.scene->updateSceneNextFrame();
                }
            }
            break;
        case MatType::GLASS:
            Label("Fuzziness", "Controls how unpolished the glass will be.\nUsed in the Cook-Torrance model with the GGX microfacets distribution.");
            selectedObject->mat.data.x = glm::clamp(selectedObject->mat.data.x, 0.0f, 0.5f);
            if (ImGui::SliderFloat("##Fuzziness", &selectedObject->mat.data.x, 0.0f, 0.5f)){
                selectedObject->mat.data.x = glm::clamp(selectedObject->mat.data.x, 0.0f, 0.5f);
                m_context.scene->updateSceneNextFrame();
            }
            Label("IOR", "Index of refraction of the dielectric.");
            selectedObject->mat.data.y = glm::clamp(selectedObject->mat.data.y, 1.0f, 2.0f);
            if (ImGui::SliderFloat("##IOR", &selectedObject->mat.data.y, 1.0f, 2.0f)){
                selectedObject->mat.data.y = glm::clamp(selectedObject->mat.data.y, 1.0f, 2.0f);
                m_context.scene->updateSceneNextFrame();
            }
            if (m_context.scene->getSpectral()){
                Label("Dispertion Factor", "How much the IOR varies based on wavelength, using Cauchy's approximation.");
                selectedObject->mat.data2.x = glm::clamp(selectedObject->mat.data2.x, 0.0f, 0.05f);
                if (ImGui::SliderFloat("##Dispertion Factor", &selectedObject->mat.data2.x, 0.0f, 0.05f)){
                    selectedObject->mat.data2.x = glm::clamp(selectedObject->mat.data2.x, 0.0f, 0.05f);
                    m_context.scene->updateSceneNextFrame();
                }
            }
            ImGui::Spacing();
            ImGui::Spacing();
            Label("Absorption Factor", "How much light is absorbed in the dielectric, using Beer-Lambert's law.");
            selectedObject->mat.data.z = glm::clamp(selectedObject->mat.data.z, 0.0f, 2.0f);
            if (ImGui::SliderFloat("##Absorption Factor", &selectedObject->mat.data.z, 0.0f, 2.0f)){
                selectedObject->mat.data.z = glm::clamp(selectedObject->mat.data.z, 0.0f, 2.0f);
                m_context.scene->updateSceneNextFrame();
            }
            Label("Scattering Factor", "How much light is scattered in the dielectric.");
            selectedObject->mat.data.w = glm::clamp(selectedObject->mat.data.w, 0.0f, 2.0f);
            if (ImGui::SliderFloat("##Scattering Factor", &selectedObject->mat.data.w, 0.0f, 2.0f)){
                selectedObject->mat.data.w = glm::clamp(selectedObject->mat.data.w, 0.0f, 2.0f);
                m_context.scene->updateSceneNextFrame();
            }
            Label("Anisotropy", "How much the ray's direction is taken into account when scattering.\ng=1: forward scattering\ng=0: isotropic scattering\ng=-1: backward scattering");
            selectedObject->mat.data2.y = glm::clamp(selectedObject->mat.data2.y, -1.0f, 1.0f);
            if (ImGui::SliderFloat("##Anisotropy", &selectedObject->mat.data2.y, -1.0f, 1.0f)){
                selectedObject->mat.data2.y = glm::clamp(selectedObject->mat.data2.y, -1.0f, 1.0f);
                m_context.scene->updateSceneNextFrame();
            }
            ImGui::Spacing();
            ImGui::Spacing();
            if (m_context.scene->getSpectral()){
                Label("Film IOR", "Index of refraction of the thin film on the surface.");
                selectedObject->mat.data2.z = glm::clamp(selectedObject->mat.data2.z, 1.0f, 2.0f);
                if (ImGui::SliderFloat("##Film IOR", &selectedObject->mat.data2.z, 1.0f, 2.0f)){
                    selectedObject->mat.data2.z = glm::clamp(selectedObject->mat.data2.z, 1.0f, 2.0f);
                    m_context.scene->updateSceneNextFrame();
                }
                Label("Film Depth", "Depth of the thin film on the surface.");
                selectedObject->mat.data2.w = glm::clamp(selectedObject->mat.data2.w, 0.0f, 1000.0f);
                if (ImGui::SliderFloat("##Film Depth", &selectedObject->mat.data2.w, 0.0f, 1000.0f)){
                    selectedObject->mat.data2.w = glm::clamp(selectedObject->mat.data2.w, 0.0f, 1000.0f);
                    m_context.scene->updateSceneNextFrame();
                }
            }
            break; 
        case MatType::GLOSSY:
            Label("Fuzziness", "Controls how unpolished the object will be.\nUsed in the Cook-Torrance model with the GGX microfacets distribution.");
            selectedObject->mat.data.x = glm::clamp(selectedObject->mat.data.x, 0.0f, 0.8f);
            if (ImGui::SliderFloat("##Fuzziness", &selectedObject->mat.data.x, 0.0f, 0.8f)){
                selectedObject->mat.data.x = glm::clamp(selectedObject->mat.data.x, 0.0f, 0.8f);
                m_context.scene->updateSceneNextFrame();
            }
            Label("Metallic");
            selectedObject->mat.data.y = glm::clamp(selectedObject->mat.data.y, 0.05f, 1.0f);
            if (ImGui::SliderFloat("##Metallic", &selectedObject->mat.data.y, 0.05f, 1.0f)){
                selectedObject->mat.data.y = glm::clamp(selectedObject->mat.data.y, 0.05f, 1.0f);
                m_context.scene->updateSceneNextFrame();
            }
            break;
        case MatType::FUR:
            Label("RoughA", "Roughness on the azimuth in degree angles.");
            selectedObject->mat.data.x = glm::clamp(selectedObject->mat.data.x, 0.0f, 1.0f);
            if (ImGui::SliderFloat("##RoughA", &selectedObject->mat.data.x, 0.0f, 1.0f)){
                selectedObject->mat.data.x = glm::clamp(selectedObject->mat.data.x, 0.0f, 1.0f);
                m_context.scene->updateSceneNextFrame();
            }
            Label("RoughL", "Roughness on the longitude in degree angles.");
            selectedObject->mat.data.y = glm::clamp(selectedObject->mat.data.y, 0.0f, 1.0f);
            if (ImGui::SliderFloat("##RoughL", &selectedObject->mat.data.y, 0.0f, 1.0f)){
                selectedObject->mat.data.y = glm::clamp(selectedObject->mat.data.y, 0.0f, 1.0f);
                m_context.scene->updateSceneNextFrame();
            }
            Label("Alpha", "Scales' tilt angle.");
            selectedObject->mat.data.z = glm::clamp(selectedObject->mat.data.z, 0.0f, 5.0f);
            if (ImGui::SliderFloat("##Alpha", &selectedObject->mat.data.z, 0.0f, 5.0f)){
                selectedObject->mat.data.z = glm::clamp(selectedObject->mat.data.z, 0.0f, 5.0f);
                m_context.scene->updateSceneNextFrame();
            }
            // Label("Radius Mult", "Radius multiplier of the hair/fur strands.");
            // selectedObject->mat.data.z = glm::clamp(selectedObject->mat.data.z, 0.0f, 5.0f);
            // if (ImGui::SliderFloat("##Radius Mult", &selectedObject->mat.data.z, 0.0f, 5.0f)){
            //     selectedObject->mat.data.z = glm::clamp(selectedObject->mat.data.z, 0.0f, 5.0f);
            //     m_context.scene->updateSceneNextFrame();
            // }
            break;
        case MatType::EMIT:
            Label("Intensity");
            selectedObject->mat.data.x = glm::max(selectedObject->mat.data.x, 0.0f);
            if (ImGui::DragFloat("##Intensity", &selectedObject->mat.data.x)){
                selectedObject->mat.data.x = glm::max(selectedObject->mat.data.x, 0.0f);
                m_context.scene->updateSceneNextFrame();
            }
            break;
        default:
            break;
    }
}

void UI::renderPopup(){
    if (m_context.scene->getSelectedObject() < 0) return;
    Object* selectedObject = m_context.scene->getObject(m_context.scene->getSelectedObject());
    
    ImGui::SetNextWindowSizeConstraints(ImVec2(300, 0), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::PushStyleColor(ImGuiCol_Border, UIColors::lightBlueBorder);
        
        ImGui::SeparatorText("Object");
        BeginTwoColumnLayout();
        Label("Position");
        if (ImGui::DragFloat3("##Position", &selectedObject->pos.x, 0.01f))
            m_context.scene->updateSceneNextFrame();
        
        Label("Rotation");
        selectedObject->rotation = mod(mod(selectedObject->rotation, 360.0f) + 360.0f, 360.0f);
        if (ImGui::DragFloat3("##Rotation", &selectedObject->rotation.x, 0.1f)){
            selectedObject->rotation = mod(mod(selectedObject->rotation, 360.0f) + 360.0f, 360.0f);
            m_context.scene->updateSceneNextFrame();
        }
        
        static bool scaleLocked = true;
        static vec3 scaleRatio = vec3(1.0f);

        bool previousScaleLocked = scaleLocked;
        bool disabled = false;
        if (selectedObject->type == PrimType::SPHERE && selectedObject->mat.type == MatType::EMIT
            || selectedObject->type == PrimType::FUR_){
            for (int i = 0; i < 3; i++)
                selectedObject->scale[i] = std::max({selectedObject->scale[0], selectedObject->scale[1], selectedObject->scale[2]});
            
            disabled = true;
            scaleLocked = true;
        }
        Label("Scale", "", [=](){
            ImGui::BeginDisabled(disabled);
            ImGui::Checkbox("##ScaleLocker", &scaleLocked);
            ImGui::EndDisabled();
        }, 24.0f);
        
        selectedObject->scale = max(selectedObject->scale, vec3(0.0f));

        if (scaleLocked && !previousScaleLocked){
            vec3 s = selectedObject->scale;
            float maxComp = std::max({s.x, s.y, s.z});
            if (maxComp > 0.0f)
                scaleRatio = s / maxComp;
            else
                scaleRatio = vec3(1.0f);
        }

        vec3 beforeDrag = selectedObject->scale;
        if (ImGui::DragFloat3("##Scale", &selectedObject->scale.x, 0.001f)){
            selectedObject->scale = max(selectedObject->scale, vec3(0.0f));
            
            if (scaleLocked){
                vec3 delta = abs(selectedObject->scale - beforeDrag);
                int changedAxis = (delta.x > delta.y && delta.x > delta.z) ? 0 : (delta.y > delta.z ? 1 : 2);
                
                float newScale = selectedObject->scale[changedAxis];
                selectedObject->scale = scaleRatio * (newScale / scaleRatio[changedAxis]);
            }
            m_context.scene->updateSceneNextFrame();
        }

        EndTwoColumnLayout();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::SeparatorText("Shape");
        BeginTwoColumnLayout();

        Label("Shape Type");
        PrimType previousType = selectedObject->type;
        if (ImGui::Combo("##Shape Type", (int*)(&selectedObject->type), Scene::primLabels, IM_ARRAYSIZE(Scene::primLabels))){
            if (previousType == PrimType::MESH_ || selectedObject->type == PrimType::MESH_){
                m_context.scene->updateMeshes();
                m_context.scene->updateFurs();
            }
            m_context.scene->updateSceneNextFrame();
        }

        if (selectedObject->type == PrimType::MESH_){
            Label("Mesh Used");
            vector<const char*> meshes = m_context.scene->getMeshNames();
            selectedObject->dataIndex = glm::clamp(selectedObject->dataIndex, 0, (int)meshes.size());
            if (ImGui::Combo("##Model Used", &selectedObject->dataIndex, meshes.data(), (int)meshes.size())){
                selectedObject->dataIndex = glm::clamp(selectedObject->dataIndex, 0, (int)meshes.size());
                m_context.scene->updateMeshes();
                m_context.scene->updateSceneNextFrame();
            }

            Label("Is Smooth");
            if (ImGui::Checkbox("##Is Smooth", &selectedObject->isSmooth))
                m_context.scene->updateSceneNextFrame();
        }
        else if (selectedObject->type == PrimType::FUR_){
            Label("Fur Used");
            vector<const char*> furs = m_context.scene->getFurNames();
            selectedObject->dataIndex = glm::clamp(selectedObject->dataIndex, 0, (int)furs.size());
            if (ImGui::Combo("##Fur Used", &selectedObject->dataIndex, furs.data(), (int)furs.size())){
                selectedObject->dataIndex = glm::clamp(selectedObject->dataIndex, 0, (int)furs.size());
                m_context.scene->updateFurs();
                m_context.scene->updateSceneNextFrame();
            }

        }
        EndTwoColumnLayout();
        
        ImGui::Spacing();
        ImGui::Spacing();

        ImGui::SeparatorText("Material");
        BeginTwoColumnLayout();
        
        const char* items[] = { 
            "Diffuse", 
            "Metal", 
            "Dielectric", 
            "Glossy",
            "Fur",
            "Emit"
        };
        Label("Material type");
        if (ImGui::Combo("##Material type", (int*)&selectedObject->mat.type, items, IM_ARRAYSIZE(items)))
            m_context.scene->updateSceneNextFrame();

        if (selectedObject->mat.type != MatType::METAL){
            Label("Color");
            if (ImGui::ColorEdit3("##Color", &selectedObject->mat.color.x))
                m_context.scene->updateSceneNextFrame();
        }
        else{
            Label("Reflectivity");
            if (ImGui::ColorEdit3("##Reflectivity", &selectedObject->mat.color.x))
                m_context.scene->updateSceneNextFrame();
            Label("Edge Tint");
            if (ImGui::ColorEdit3("##Edge Tint", &selectedObject->mat.color2.x))
                m_context.scene->updateSceneNextFrame();
        }

        renderPopupData(selectedObject);
        EndTwoColumnLayout();
        
        ImGui::Spacing();
        ImGui::Spacing();
        
        ImGui::NewLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.3f, 0.3f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.3f, 0.3f, 0.4f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 0.6f));
        if (ImGui::Button("Remove", ImVec2(-1, 0))){ 
            m_context.scene->removeObject(m_context.scene->getSelectedObject());
        }
        ImGui::PopStyleColor(4);
    }
    ImGui::End();
}

int UI::renderListItems(const vector<const char*>& items, int* selectedIndex){
    ImGui::PushStyleColor(ImGuiCol_Header, UIColors::fgColor);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(UIColors::fgColor.x, UIColors::fgColor.y, UIColors::fgColor.z, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, UIColors::fgColor);
    ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.04f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    float itemHeight = ImGui::GetTextLineHeight() + 8.0f;
    int newSelected = -1;
    for (int i = 0; i < (int)items.size(); i++){
        bool isSelected = (*selectedIndex == i);
        
        if (ImGui::Selectable(items[i], isSelected, ImGuiSelectableFlags_None, ImVec2(ImGui::GetContentRegionAvail().x, itemHeight))){
            *selectedIndex = i;
            newSelected = i;
        }
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
    return newSelected;
}

void UI::renderScene(){
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, UIColors::bgColor);
    ImGui::SetNextWindowSizeConstraints(ImVec2(-FLT_MIN, 250.0f), ImVec2(-FLT_MIN, ImGui::GetIO().DisplaySize.y - 250.0f));
    ImGui::BeginChild("SceneContainer", ImVec2(-FLT_MIN, 200.0f), ImGuiChildFlags_ResizeY | ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::PopStyleVar(1);
    ImGui::PopStyleColor(1);

    ImGui::BeginChild("Scene", ImVec2(-FLT_MIN, -FLT_MIN - 2), ImGuiChildFlags_Borders), ImGuiWindowFlags_AlwaysVerticalScrollbar;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 4.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 7.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, UIColors::lightBlueBorder);

    float buttonRegionSize = 30.0f;
    if (ImGui::BeginTabBar("SceneTabs")){
        if (ImGui::BeginTabItem("Scene")){
            ImGui::BeginChild("SceneListContainer", ImVec2(-FLT_MIN -buttonRegionSize, -FLT_MIN), 0, ImGuiWindowFlags_AlwaysVerticalScrollbar);
            ImGui::Dummy(ImVec2(-FLT_MIN, 4.0f));
            static int selectedSceneIndex = -1;
            selectedSceneIndex = m_context.scene->getSelectedObject();
            vector<const char*> items = m_context.scene->getObjectNames();
            int newSelected = renderListItems(items, &selectedSceneIndex);
            if (newSelected >= 0) m_context.scene->selectObject(newSelected);
            ImGui::EndChild();
            if (ImGui::IsItemClicked()) m_context.scene->selectObject(-1);
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_ChildBg, UIColors::bgColor);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
            ImGui::BeginChild("SceneButtonsContainer", ImVec2(-FLT_MIN-2, -FLT_MIN), ImGuiChildFlags_AlwaysUseWindowPadding);

            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(UIColors::mgColor.x, UIColors::mgColor.y, UIColors::mgColor.z, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(UIColors::mgColor.x, UIColors::mgColor.y, UIColors::mgColor.z, 0.3f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(UIColors::mgColor.x, UIColors::mgColor.y, UIColors::mgColor.z, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_Separator, UIColors::mgColor);
            if (ImGui::Button("+", ImVec2(buttonRegionSize-2, buttonRegionSize-2))){
                m_context.scene->addObject(Object());
            }
            ImGui::Separator();
            if (ImGui::Button("-", ImVec2(buttonRegionSize-2, buttonRegionSize-2))){
                m_context.scene->removeObject(m_context.scene->getSelectedObject());
            }
            ImGui::Separator();
            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar(2);

            ImGui::EndChild();
            ImGui::PopStyleColor(1);
            ImGui::PopStyleVar(1);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Models")){
            ImGui::BeginChild("SceneListContainer", ImVec2(-FLT_MIN -buttonRegionSize, -FLT_MIN), 0, ImGuiWindowFlags_AlwaysVerticalScrollbar);
            ImGui::Dummy(ImVec2(-FLT_MIN, 4.0f));
            static int selectedMeshesIndex = -1;
            selectedMeshesIndex = m_context.scene->getSelectedMesh();
            vector<const char*> items = m_context.scene->getMeshNames();
            int newSelected = renderListItems(items, &selectedMeshesIndex);
            if (newSelected >= 0) m_context.scene->selectMesh(newSelected);
            ImGui::EndChild();
            if (ImGui::IsItemClicked()) m_context.scene->selectMesh(-1);
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_ChildBg, UIColors::bgColor);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
            ImGui::BeginChild("SceneButtonsContainer", ImVec2(-FLT_MIN-2, -FLT_MIN), ImGuiChildFlags_AlwaysUseWindowPadding);

            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(UIColors::mgColor.x, UIColors::mgColor.y, UIColors::mgColor.z, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(UIColors::mgColor.x, UIColors::mgColor.y, UIColors::mgColor.z, 0.3f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(UIColors::mgColor.x, UIColors::mgColor.y, UIColors::mgColor.z, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_Separator, UIColors::mgColor);
            if (ImGui::Button("+", ImVec2(buttonRegionSize-2, buttonRegionSize-2))){
                importModelDialog();
            }
            ImGui::Separator();
            if (ImGui::Button("-", ImVec2(buttonRegionSize-2, buttonRegionSize-2))){
                m_context.scene->removeMesh(m_context.scene->getSelectedMesh());
            }
            ImGui::Separator();
            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar(2);

            ImGui::EndChild();
            ImGui::PopStyleColor(1);
            ImGui::PopStyleVar(1);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Furs")){
            ImGui::BeginChild("SceneListContainer", ImVec2(-FLT_MIN -buttonRegionSize, -FLT_MIN), 0, ImGuiWindowFlags_AlwaysVerticalScrollbar);
            ImGui::Dummy(ImVec2(-FLT_MIN, 4.0f));
            static int selectedFursIndex = -1;
            selectedFursIndex = m_context.scene->getSelectedFur();
            vector<const char*> items = m_context.scene->getFurNames();
            int newSelected = renderListItems(items, &selectedFursIndex);
            if (newSelected >= 0) m_context.scene->selectFur(newSelected);
            ImGui::EndChild();
            if (ImGui::IsItemClicked()) m_context.scene->selectFur(-1);
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_ChildBg, UIColors::bgColor);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
            ImGui::BeginChild("SceneButtonsContainer", ImVec2(-FLT_MIN-2, -FLT_MIN), ImGuiChildFlags_AlwaysUseWindowPadding);

            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(UIColors::mgColor.x, UIColors::mgColor.y, UIColors::mgColor.z, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(UIColors::mgColor.x, UIColors::mgColor.y, UIColors::mgColor.z, 0.3f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(UIColors::mgColor.x, UIColors::mgColor.y, UIColors::mgColor.z, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_Separator, UIColors::mgColor);
            if (ImGui::Button("+", ImVec2(buttonRegionSize-2, buttonRegionSize-2))){
                importFurDialog();
            }
            ImGui::Separator();
            if (ImGui::Button("-", ImVec2(buttonRegionSize-2, buttonRegionSize-2))){
                m_context.scene->removeFur(m_context.scene->getSelectedFur());
            }
            ImGui::Separator();
            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar(2);

            ImGui::EndChild();
            ImGui::PopStyleColor(1);
            ImGui::PopStyleVar(1);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(2);
    ImGui::EndChild();
    ImGui::EndChild();
}

void UI::renderParameters(){
    ImGui::BeginChild("Parameters", ImVec2(-FLT_MIN, -FLT_MIN), ImGuiChildFlags_Borders);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 4.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, UIColors::lightBlueBorder);

    float padding = ImGui::GetStyle().WindowPadding.y;
    float buttonHeight = 40;

    
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 6.0f);
    ImGui::BeginChild("Scrollable Parameters", ImVec2(-FLT_MIN, -(buttonHeight + padding)), ImGuiChildFlags_None, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    if (BeginCustomHeader("Camera Settings")){
        BeginTwoColumnLayout();
        CameraProperties* camProps = m_context.camera->getCameraProperties();

        Label("Fov");
        if (ImGui::SliderFloat("##Fov", &camProps->fov, 20.0f, 90.0f))
            m_context.resetFrame();
        
        Label("Aperture");
        if (ImGui::SliderFloat("##Aperture", &camProps->aperture, 0.0f, 1.0f))
            m_context.resetFrame();
        
        Label("Focal Length");
        if (ImGui::DragFloat("##Focal Length", &camProps->focalLength, 0.1f, -1.0f, FLT_MAX))
            m_context.resetFrame();

        EndTwoColumnLayout();
        ImGui::TreePop();
    }
    EndCustomHeader();
    
    if (BeginCustomHeader("Sky Box Settings")){
        BeginTwoColumnLayout();

        Label("Sky Intensity");
        if (ImGui::SliderFloat("##Sky Intensity", &m_context.scene->skyIntensity, 0.0f, 3.0f))
            m_context.scene->updateSceneNextFrame();
        
        Label("Sky Top Color");
        if (ImGui::ColorEdit3("##Sky Top Color", &m_context.scene->skyTopColor.x))
            m_context.scene->updateSceneNextFrame();
            
        Label("Sky Middle Color");
        if (ImGui::ColorEdit3("##Sky Middle Color", &m_context.scene->skyMiddleColor.x))
            m_context.scene->updateSceneNextFrame();
            
        Label("Sky Bottom Color");
        if (ImGui::ColorEdit3("##Sky Bottom Color", &m_context.scene->skyBottomColor.x))
            m_context.scene->updateSceneNextFrame();

        EndTwoColumnLayout();
        ImGui::TreePop();
    }
    EndCustomHeader();

    if (BeginCustomHeader("BVH Settings")){
        BeginTwoColumnLayout();

        Label("Debug Visualization Mode");
        if (ImGui::Checkbox("##Debug Visualization Mode", &m_debugBVH))
            m_context.resetFrame();

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
        ImGui::BeginDisabled(!m_context.animator->canGoToPreviousKeyFrame());
        if(ImGui::Button("Previous Key", size)){
            m_context.animator->previousKeyFrame();
        }
        ImGui::EndDisabled();

        ImGui::TableSetColumnIndex(1);
        ImGui::BeginDisabled(!m_context.animator->canGoToNextKeyFrame());
        if(ImGui::Button("Next Key", size)){
            m_context.animator->nextKeyFrame();
        }
        ImGui::EndDisabled();

        EndTwoColumnLayout();

        string keyFrameLabel = "Add Keyframe";
        if (m_context.animator->isOnKeyFrame()) keyFrameLabel = "Update Keyframe";
        if(ImGui::Button(keyFrameLabel.c_str(), ImVec2(-FLT_MIN, 0))){
            m_context.animator->addKeyFrame();
        }

        ImGui::Spacing();
        ImGui::SetNextItemWidth(-FLT_MIN);
        m_animationTime = m_context.animator->getAnimationTime();
        if (ImGui::SliderFloat("##Timeline", &m_animationTime, 0, 1, "")) {
            m_context.animator->updateCurrentKeyFrame(m_animationTime);
        }
        ImVec2 minRect = ImGui::GetItemRectMin();
        ImVec2 maxRect = ImGui::GetItemRectMax();
        vector<float> keyPoses = m_context.animator->getKeyPoses();
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
            m_context.renderer->startRendering(m_renderSamples, "result.png", true, true);
        }

        if (ImGui::Button("Render Image", ImVec2(-FLT_MIN, 0))){
            m_context.renderer->startRendering(m_renderSamples);
        }

        if (ImGui::Button("Render Animation", ImVec2(-FLT_MIN, 0))){
            m_context.animator->start(m_renderSamples, m_animationFPS, m_animationDuration);
            m_context.resetFrame();
        }
       
        if (m_context.renderer->isRendering()) {
            int progress = m_context.renderer->getProgress();
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
        if (ImGui::InputInt("##Max Bounces", &m_maxBounces)) m_context.resetFrame();
        
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
    ImGui::PopStyleVar(1);
    
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - buttonHeight - padding);
    bool isSpectral = m_context.scene->getSpectral();
    string spectralButtonLabel = "Toggle Spectral : " + string(isSpectral ? "ON" : "OFF");
    if (ImGui::Button(spectralButtonLabel.c_str(), ImVec2(-FLT_MIN, buttonHeight))){
        m_context.scene->setSpectral(!isSpectral);
        m_context.reloadShader();
    }

    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(1);
    ImGui::EndChild();
}

void UI::importModelDialog(){
    bool cancel;
    nfdopendialogu8args_t args = {0};
    nfdfilteritem_t filters[] = {
        { "3D Models", "obj" },
        { "OBJ", "obj" }
    };
    args.filterList = filters;
    args.filterCount = 2;
    string path = m_context.app->openFileDialog(cancel, args);
    if (cancel) return;

    shared_ptr<Mesh> newMesh = make_shared<Mesh>();
    newMesh->loadFromModel(path.c_str());
    m_context.scene->addMesh(newMesh);
}

void UI::importFurDialog(){
    bool cancel;
    nfdopendialogu8args_t args = {0};
    nfdfilteritem_t filters[] = {
        { "Fur Models", "bin" },
        { "bin" }
    };
    args.filterList = filters;
    args.filterCount = 1;
    string path = m_context.app->openFileDialog(cancel, args);
    if (cancel) return;

    shared_ptr<Fur> newFur = make_shared<Fur>();
    newFur->loadFromBin(path.c_str());
    m_context.scene->addFur(newFur);
}

void UI::renderMainMenuBar(){
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, UIColors::bgColor);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Import Model (.obj)"))  {
                importModelDialog();
            }
            if (ImGui::MenuItem("Import Fur (.bin)"))  {
                importFurDialog();
            }
            if (ImGui::MenuItem("Save Scene (.json)")) {
                bool cancel;
                string path = m_context.app->saveFileDialog(cancel, "scene.json");
                m_context.scene->stateToJson(m_context.scene->getState(), path);
            }
            if (ImGui::MenuItem("Load Scene (.json)"))  {
                bool cancel;
                nfdopendialogu8args_t args = {0};
                nfdfilteritem_t filters[] = {
                    { "JSON", "json" },
                    { "json" }
                };
                args.filterList = filters;
                args.filterCount = 1;
                string path = m_context.app->openFileDialog(cancel, args);
                m_context.scene->loadFromState(m_context.scene->stateFromJson(path));
                m_context.animator->clear();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit"))  {
                glfwSetWindowShouldClose(m_context.app->getWindow(), true);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(1);
}

void UI::render() {
    renderMainMenuBar();

    float menuBarHeight = ImGui::GetMainViewport()->WorkPos.y;
    float workingFrameHeight = ImGui::GetMainViewport()->WorkSize.y;

    ImGui::PushStyleColor(ImGuiCol_Header, UIColors::mgColor);
    ImGui::PushStyleColor(ImGuiCol_TitleBg, UIColors::mgColor);
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, UIColors::mgColor);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, UIColors::mgColor);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 0.0f);
    
        ImGui::PushStyleColor(ImGuiCol_WindowBg, UIColors::fgColor);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
        
            renderStats();
            renderGizmos();
            if (!m_show){
                renderPointer();
                ImGui::PopStyleColor(5);
                ImGui::PopStyleVar(5);
                return;
            }
            
        ImGui::BeginDisabled(m_context.renderer->isRendering() || m_disabled);

            renderPopup();
        
        ImGui::PopStyleColor(1);
        ImGui::PopStyleVar(1);

        
        
        ImGui::PushStyleColor(ImGuiCol_WindowBg, UIColors::bgColor);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, UIColors::mgColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.1f, 0.1f, 0.1f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.1f, 0.1f, 0.1f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 1.0f));
        
            ImGui::SetNextWindowPos(ImVec2(0, menuBarHeight), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(400, workingFrameHeight), ImGuiCond_Once);
            ImGui::SetNextWindowSizeConstraints(ImVec2(300, workingFrameHeight), 
                                                ImVec2(ImGui::GetIO().DisplaySize.x / 2.0f, workingFrameHeight));
            ImGui::Begin("Left Window", (bool*)NULL, ImGuiWindowFlags_NoCollapse 
                                                    | ImGuiWindowFlags_NoTitleBar 
                                                    | ImGuiWindowFlags_NoBringToFrontOnFocus);
                                                    
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            renderScene();
            ImGui::PopStyleVar(1);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
            renderParameters();
            ImGui::PopStyleVar(1);

            ImGui::End();
        
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(4);

        ImGui::EndDisabled();

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(4);
}

void UI::updateGPU() const {
    glUniform1i(ShaderProgram::getVarLoc("maxBounces"), m_maxBounces);

    glUniform1i(ShaderProgram::getVarLoc("debugBVH"), m_debugBVH ? 1 : 0);
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