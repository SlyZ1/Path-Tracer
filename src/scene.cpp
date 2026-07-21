#include "scene.hpp"
#include <cfloat>

using namespace glm;

glm::vec3 Scene::m_cameraDirection = glm::vec3(0, 0, -1);
glm::vec3 Scene::m_cameraPosition  = glm::vec3(0, 0, 0);

const char* Scene::primLabels[5] = {
  "Sphere",
  "Plane",
  "Cube",
  "Cylinder",
  "Mesh",
};

shared_ptr<Scene> Scene::defaultScene(shared_ptr<App> app, shared_ptr<Camera> camera, function<void()> resetFrame){
    shared_ptr<Scene> scene = make_shared<Scene>(app, camera, resetFrame);
    scene->initGPU();
    
    Object plane;
    plane.type = PrimType::PLANE;
    plane.pos = vec3(0.0f);
    plane.scale = vec3(30.0f);
    plane.rotation = vec3(0.0f);
    plane.mat = Material::glossyMaterial(vec3(1.0f), 0.0f, 0.2f);
    scene->addObject(plane);

    Object light;
    light.type = PrimType::SPHERE;
    light.pos = vec3(0.0f, 5.0f, 2.0f);
    light.scale = vec3(1.0f);
    light.rotation = vec3(0.0f);
    light.mat = Material::emitMaterial(vec3(1.0f), 20.0f);
    scene->addObject(light); 

    shared_ptr<Mesh> bunnyMesh = make_shared<Mesh>();
    bunnyMesh->loadFromModel("models/bunny.obj");
    int bunnyIndex = scene->addMesh(bunnyMesh); 

    Object meshObject;
    meshObject.pos = vec3(0.0f, 1.29f, 0.0f);
    meshObject.scale = vec3(3.0f);
    meshObject.rotation = vec3(0.0f);
    meshObject.type = PrimType::MESH_;
    meshObject.mat = Material::diffuseMaterial(vec3(0.9f, 0.6f, 0.2f), 0.0f);
    meshObject.meshIndex = bunnyIndex;
    meshObject.isSmooth = true;
    scene->addObject(meshObject);

    scene->selectObject(-1);

    return scene;
}

Ray Scene::rayFromClick(shared_ptr<Camera> camera, glm::vec2 screenPos){
    vec3 origin = camera->position();
    vec3 dir = camera->lookDir();
    float fovDegrees = camera->getCameraProperties()->fov;

    Ray ray;
    ray.origin = origin;
    m_cameraDirection = normalize(dir);
    m_cameraPosition = origin;

    float fov = radians(fovDegrees);
    vec3 forward = m_cameraDirection;

    vec3 worldUp = abs(forward.y) < 0.999
                 ? vec3(0,1,0)
                 : vec3(0,0,1);

    vec3 right = normalize(cross(forward, worldUp));
    vec3 up = cross(right, forward);

    float tanHalfFov = tan(fov * 0.5f);

    vec3 newDir = forward + (right * screenPos.x + up * screenPos.y) * tanHalfFov;
    ray.direction = normalize(newDir);
    return ray;
}

SceneState Scene::stateFromJson(const string& path){
    if (fs::path(path).extension().string() != ".json") return SceneState();
    std::ifstream f(path);
    json data = json::parse(f);
    f.close();
    return data.get<SceneState>();
}

void Scene::stateToJson(SceneState sceneState, const string& path){
    // Remove unused meshes and remap mesh indices
    vector<string> utilizedMeshes;
    vector<int> meshIsUsed = vector<int>(sceneState.modelPaths.size(), -1);
    for (Object& obj : sceneState.objectStates){
        if (obj.type == PrimType::MESH_){
            int index = obj.meshIndex;
            if (meshIsUsed[index] >= 0){
                obj.meshIndex = meshIsUsed[index];
            }
            else{
                utilizedMeshes.push_back(sceneState.modelPaths[index]);
                obj.meshIndex = (int)utilizedMeshes.size() - 1;
                meshIsUsed[index] = obj.meshIndex;
            }
        }
    }
    sceneState.modelPaths = utilizedMeshes;

    json data = json(sceneState);
    ofstream f(path);
    f << data.dump(4);
    f.close();
    cout << "Scene saved to " << path << "." << endl;
}

shared_ptr<Mesh> Scene::findMesh(const string& path){
    for (shared_ptr<Mesh> mesh : m_meshes){
        if (mesh->modelPath == path) return mesh;
    }
    return nullptr;
}

vector<const char*> Scene::getObjectNames() const {
    vector<const char*> names = {};
    for (const Object& obj : m_objects){
        names.push_back(obj.name.c_str());
    }
    return names; 
}

void Scene::loadFromState(const SceneState& sceneState, bool verbose){
    if ((int)sceneState.objectStates.empty()) return;

    vector<shared_ptr<Mesh>> meshes;
    for (const string& path: sceneState.modelPaths){
        shared_ptr<Mesh> newMesh = findMesh(path);
        if (newMesh == nullptr){
            newMesh = make_shared<Mesh>();
            newMesh->loadFromModel(path.c_str());
        }
        meshes.push_back(newMesh);
    }

    deleteScene();

    m_meshes = meshes;
    m_objects = sceneState.objectStates;
    m_selectedObject = -1;
    m_copiedObject = -1;

    skyBottomColor = sceneState.skyBottom;
    skyMiddleColor = sceneState.skyMiddle;
    skyTopColor = sceneState.skyTop;
    skyIntensity = sceneState.skyIntensity;

    CameraProperties* camProps = m_camera->getCameraProperties();
    *camProps = sceneState.camProperties;

    updateScene();
    if (verbose) cout << "Scene loaded." << endl;
}

SceneState Scene::getState(){
    SceneState sceneState;
    sceneState.modelPaths = {};
    sceneState.objectStates = m_objects;
    for (auto mesh : m_meshes){
        sceneState.modelPaths.push_back(mesh->modelPath);
    }

    sceneState.skyBottom = skyBottomColor;
    sceneState.skyMiddle = skyMiddleColor;
    sceneState.skyTop = skyTopColor;
    sceneState.skyIntensity = skyIntensity;
    sceneState.camProperties = *(m_camera->getCameraProperties());
    return sceneState;
}

shared_ptr<Object> Scene::getObjectFromId(SceneState sceneState, unsigned int ID){
    for (const Object& obj : sceneState.objectStates){
        if (obj.ID == ID) return make_shared<Object>(obj);
    }
    return nullptr;
}


glm::vec2 Scene::worldToScreen(shared_ptr<Camera> camera, glm::vec3 worldPos){
    vec3 forward = camera->lookDir();

    vec3 worldUp = abs(forward.y) < 0.999
                 ? vec3(0,1,0)
                 : vec3(0,0,1);

    vec3 right = normalize(cross(forward, worldUp));
    vec3 up = cross(right, forward);

    vec3 dir = normalize(worldPos - camera->position());
    float normFactor = 1 / dot(forward, dir);
    dir *= normFactor;
    float fov = m_camera->getCameraProperties()->fov;
    float tanHalfFov = tan(radians(fov) * 0.5f);
    float rightCompo = dot(right, dir) / tanHalfFov;
    float upCompo = -dot(up, dir) / tanHalfFov;
    return vec2(rightCompo * m_app->height() / 2.0f, upCompo * m_app->height() / 2.0f);
}

void Scene::initGPU(){
    glDeleteBuffers(1, &m_sceneBuffer);
    glGenBuffers(1, &m_sceneBuffer);
    glDeleteBuffers(1, &m_materialsBuffer);
    glGenBuffers(1, &m_materialsBuffer);
    glDeleteBuffers(1, &m_indicesBuffer);
    glGenBuffers(1, &m_indicesBuffer);
    glDeleteBuffers(1, &m_meshInfosBuffer);
    glGenBuffers(1, &m_meshInfosBuffer);
    glDeleteBuffers(1, &m_trianglesBuffer);
    glGenBuffers(1, &m_trianglesBuffer);
    glDeleteBuffers(1, &m_nodesBuffer);
    glGenBuffers(1, &m_nodesBuffer);
}

int Scene::intersectObject(const Ray& ray){
    float distance = FLT_MAX;
    int intersected = -1;
    for(int i = 0; i < (int)m_objects.size(); i++){
        float dist = -1;
        Object obj = m_objects[i];
        switch(obj.type){
            case PrimType::SPHERE:
                dist = Intersections::intersectSphere(ray, obj.pos, obj.scale, obj.rotation);
                break;
            case PrimType::PLANE:
                dist = Intersections::intersectPlane(ray, obj.pos, obj.scale, obj.rotation);
                break;
            case PrimType::CUBE:
                dist = Intersections::intersectCube(ray, obj.pos, obj.scale, obj.rotation);
                break;
            case PrimType::CYLINDER:
                dist = Intersections::intersectCylinder(ray, obj.pos, obj.scale, obj.rotation);
                break;
            case PrimType::MESH_: {
                if (obj.meshIndex < 0 || obj.meshIndex >= (int)m_meshes.size()) break;
                shared_ptr<Mesh> mesh = m_meshes[obj.meshIndex];
                dist = Intersections::intersectMesh(ray, mesh, obj.pos, obj.scale, obj.rotation);
                break;
            }
            default:
                break;
        }
        if (dist >= 0 && dist < distance){
            distance = dist;
            intersected = i;
        }
    }
    return intersected;
}

int Scene::addMesh(shared_ptr<Mesh> newMesh){
    m_meshes.push_back(newMesh);
    m_numMeshesChanged = true;
    return (int)m_meshes.size() - 1;
}

void Scene::removeMesh(int index){
    if (index < 0 || index > (int)m_meshes.size()) return;
    for (Object& obj : m_objects){
        if (obj.type != PrimType::MESH_) continue;
        if (obj.meshIndex > index) obj.meshIndex -= 1;
        if (obj.meshIndex == index) obj.meshIndex = -1;
    }
    m_meshes.erase(m_meshes.begin() + index);
    updateScene();
}

int Scene::addObject(Object obj){
    obj.ID = m_maxId++;
    if (obj.type == PrimType::MESH_){
        obj.name = m_meshes[obj.meshIndex]->modelName + string(" ") + to_string(obj.ID);
    }
    else{
        obj.name = string("Primitive ") + to_string(obj.ID);
    }
    int newIndex = (int)m_objects.size();
    m_objects.push_back(obj);
    updateScene();
    m_selectedObject = newIndex;
    return newIndex;
}

Object* Scene::getObject(int index){
    if (index < 0 || index >= (int)m_objects.size()) return nullptr;

    return &m_objects[index];
}

void Scene::removeObject(int index){
    if (index < 0 || index >= (int)m_objects.size()) return;
    m_objects.erase(m_objects.begin() + index);
    updateScene();
    m_selectedObject = -1;
}

void Scene::copyObject(int index){
    m_copiedObject = index;
}

int Scene::pasteObject(){
    Object* newObj = getObject(m_copiedObject);
    if (newObj == nullptr) return -1;
    return addObject(*newObj);
}

vector<const char*> Scene::getMeshNames() const {
    vector<const char*> result;
    for (shared_ptr<Mesh> mesh : m_meshes){
        result.push_back(mesh->modelName.c_str());
    }
    return result;
}

void Scene::updateMeshes(){
    m_numMeshesChanged = true;
}

void Scene::updateScene(){
    m_sceneChanged = true;
}

void Scene::updateSceneNextFrame(){
    m_updateNextFrame = true;
}

int Scene::addMaterial(vector<Material>& materials, Material mat){ 
    for (int i = 0; i < (int)materials.size(); i++){
        const Material& material = materials[i]; 
        if (mat == material) return i; 
    }
    if (m_spectral){
        mat.color = Material::rgbToSigmoidCoeffs(mat.color);
        mat.color2 = Material::rgbToSigmoidCoeffs(mat.color2);
    }
    materials.push_back(mat);
    return (int)materials.size() - 1;
}

void Scene::updateGPU(){
    bool sceneChanged = m_sceneChanged;
    m_sceneChanged = false;
    if (m_updateNextFrame){
        m_updateNextFrame = false;
        m_sceneChanged = true;
    }
    if (!sceneChanged) return;

    m_resetFrame();

    vector<PrimitiveObject> primitives = {};
    vector<MeshInfos> meshInfos = {};
    vector<Triangle> triangles = {};
    vector<linBVHNode> nodes = {};
    int numVolumeMeshes = 0;
    int numVolumePrims = 0;
    vector<Material> materials = {};
    vector<int> volumeIndicies = {};
    vector<int> lightIndicies = {};
    vector<int> triangleOffsets = {};
    vector<int> nodeOffsets = {};
    vector<int> numberOfNodes = {};
    vector<bool> meshIsUsed = vector<bool>(m_meshes.size(), false);
    for (Object& obj : m_objects){
        if (obj.type != PrimType::MESH_ || obj.meshIndex < 0 || obj.meshIndex > (int)m_meshes.size()) continue;
        obj.meshIndex = std::min(obj.meshIndex, (int)m_meshes.size() - 1);
        meshIsUsed[obj.meshIndex] = true; 
    }
    for (int i = 0; i < (int)m_meshes.size(); i++){
        if (!meshIsUsed[i]){
            triangleOffsets.push_back(-1);
            nodeOffsets.push_back(-1);
            numberOfNodes.push_back(-1);
            continue;
        }
        shared_ptr<Mesh> mesh = m_meshes[i];

        const vector<Triangle>& meshTriangles = mesh->getTriangles();
        const vector<linBVHNode>& meshNodes = mesh->getLinNodes();

        triangleOffsets.push_back((int)triangles.size());
        nodeOffsets.push_back((int)nodes.size());
        numberOfNodes.push_back((int)meshNodes.size());

        triangles.insert(triangles.end(), meshTriangles.begin(), meshTriangles.end());
        nodes.insert(nodes.end(), meshNodes.begin(), meshNodes.end());
    }
    for (const Object& obj : m_objects){
        int matIndex = addMaterial(materials, obj.mat);
        if (obj.type == PrimType::MESH_){
            if (obj.meshIndex < 0 || obj.meshIndex > (int)m_meshes.size()) continue;
            if (obj.mat.type == MatType::GLASS && (obj.mat.data.w > 0)){
                volumeIndicies.insert(volumeIndicies.begin() + numVolumeMeshes, (int)meshInfos.size());
                numVolumeMeshes++;
            }
            MeshInfos meshInfo;
            meshInfo.triangleOffset = triangleOffsets[obj.meshIndex];
            meshInfo.nodeOffset = nodeOffsets[obj.meshIndex];
            meshInfo.numberOfNodes = numberOfNodes[obj.meshIndex];
            meshInfo.pos = obj.pos;
            meshInfo.scale = obj.scale;
            meshInfo.rotation = obj.rotation;
            meshInfo.matIndex = matIndex;
            meshInfo.isSmooth = obj.isSmooth;
            meshInfos.push_back(meshInfo);
        }
        else {
            if (obj.mat.type == MatType::GLASS && (obj.mat.data.w > 0)){
                volumeIndicies.push_back((int)primitives.size());
                numVolumePrims++;
            }
            if (obj.mat.type == MatType::EMIT) lightIndicies.push_back((int)primitives.size());
            PrimitiveObject prim;
            prim.pos = obj.pos;
            prim.scale = obj.scale;
            prim.rotation = obj.rotation;
            prim.matIndex = matIndex;
            prim.type = obj.type;
            primitives.push_back(prim);
        }
    }
    vector<int> indices = lightIndicies;
    indices.insert(indices.end(), volumeIndicies.begin(), volumeIndicies.end());

    glUniform1i(ShaderProgram::getVarLoc("numPrimitives"), (int)primitives.size());

    glUniform1i(ShaderProgram::getVarLoc("numLights"), (int)lightIndicies.size());
    glUniform1i(ShaderProgram::getVarLoc("numVolumesMeshes"), numVolumeMeshes);
    glUniform1i(ShaderProgram::getVarLoc("numVolumesPrims"), numVolumePrims);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_sceneBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, (int)primitives.size() * sizeof(PrimitiveObject), primitives.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_sceneBuffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_indicesBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, (int)indices.size() * sizeof(int), indices.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_indicesBuffer);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_materialsBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, (int)materials.size() * sizeof(Material), materials.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, m_materialsBuffer);

    if (m_numMeshesChanged){
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_trianglesBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, (int)triangles.size() * sizeof(Triangle), triangles.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_trianglesBuffer);
        glUniform1i(ShaderProgram::getVarLoc("numTriangles"), (int)triangles.size());
    
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_nodesBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, (int)nodes.size() * sizeof(linBVHNode), nodes.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_nodesBuffer);
        glUniform1i(ShaderProgram::getVarLoc("numBVHNodes"), (int)nodes.size());
        m_numMeshesChanged = false;
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_meshInfosBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, (int)meshInfos.size() * sizeof(MeshInfos), meshInfos.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_meshInfosBuffer);
    glUniform1i(ShaderProgram::getVarLoc("numMeshes"), (int)meshInfos.size());

    glUniform1f(ShaderProgram::getVarLoc("skyIntensity"), skyIntensity);
    vec3 skyBottom = skyBottomColor;
    vec3 skyMiddle = skyMiddleColor;
    vec3 skyTop = skyTopColor;
    if (m_spectral){
        skyBottom = Material::rgbToSigmoidCoeffs(skyBottomColor);
        skyMiddle = Material::rgbToSigmoidCoeffs(skyMiddleColor);
        skyTop = Material::rgbToSigmoidCoeffs(skyTopColor);
    }
    glUniform3f(ShaderProgram::getVarLoc("skyBottomColor"), skyBottom.x, skyBottom.y, skyBottom.z);
    glUniform3f(ShaderProgram::getVarLoc("skyMiddleColor"), skyMiddle.x, skyMiddle.y, skyMiddle.z);
    glUniform3f(ShaderProgram::getVarLoc("skyTopColor"), skyTop.x, skyTop.y, skyTop.z);
}

void Scene::deleteScene(){
    m_objects.clear();
    m_meshes.clear();
}