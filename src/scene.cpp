#include "scene.hpp"
#include <cfloat>

using namespace glm;

const char* Scene::primLabels[6] = {
  "Sphere",
  "Plane",
  "Cube",
  "Cylinder",
  "Mesh",
  "Fur",
};

shared_ptr<Scene> Scene::defaultScene(shared_ptr<App> app, shared_ptr<Camera> camera, function<void()> resetFrame){
    shared_ptr<Scene> scene = make_shared<Scene>(app, camera, resetFrame);
    scene->initGPU();
    scene->loadFromState(scene->stateFromJson("scenes/hair_scene.json")); 

    
    // Object plane;
    // plane.type = PrimType::PLANE;
    // plane.pos = vec3(0.0f, -1.5f, 0.0f);
    // plane.scale = vec3(30.0f); 
    // plane.rotation = vec3(0.0f);
    // plane.mat = Material::glassMaterial(vec3(0.9f, 0.5f, 0.2f), 0.2f, 1.7f, 0, 0.072f, 0, 0);
    // scene->addObject(plane);

    // Object light; 
    // light.type = PrimType::SPHERE;
    // light.pos = vec3(0.0f, 5.0f, 2.0f);
    // light.scale = vec3(1.0f);
    // light.rotation = vec3(0.0f);
    // light.mat = Material::emitMaterial(vec3(1.0f), 20.0f);
    // scene->addObject(light); 

    // shared_ptr<Fur> fur = make_shared<Fur>();
    // fur->loadFromBin("src/python/curly_lock.bin");
    // scene->addFur(fur);

    // // shared_ptr<Mesh> bunnyMesh = make_shared<Mesh>();
    // // bunnyMesh->loadFromModel("models/bunny.obj");
    // // int bunnyIndex = scene->addMesh(bunnyMesh); 

    // // Object meshObject;
    // // meshObject.pos = vec3(0.0f, 1.29f, 0.0f);
    // // meshObject.scale = vec3(3.0f);
    // // meshObject.rotation = vec3(0.0f);
    // // meshObject.type = PrimType::MESH_;
    // // meshObject.mat = Material::diffuseMaterial(vec3(0.9f, 0.6f, 0.2f), 0.0f);
    // // meshObject.dataIndex = bunnyIndex;
    // // meshObject.data = true;
    // // scene->addObject(meshObject);

    // scene->selectObject(-1);

    return scene;
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
    vector<string> utilizedMeshes = {};
    vector<string> utilizedFurs = {};
    vector<int> meshIsUsed = vector<int>(sceneState.modelPaths.size(), -1);
    vector<int> furIsUsed = vector<int>(sceneState.furPaths.size(), -1);
    for (Object& obj : sceneState.objectStates){
        int index = obj.dataIndex;
        if (obj.type == PrimType::MESH_){
            if (meshIsUsed[index] >= 0){
                obj.dataIndex = meshIsUsed[index];
            }
            else{
                utilizedMeshes.push_back(sceneState.modelPaths[index]);
                obj.dataIndex = (int)utilizedMeshes.size() - 1;
                meshIsUsed[index] = obj.dataIndex;
            }
        }
        else if (obj.type == PrimType::FUR_){
            if (furIsUsed[index] >= 0){
                obj.dataIndex = furIsUsed[index];
            }
            else{
                utilizedFurs.push_back(sceneState.furPaths[index]);
                obj.dataIndex = (int)utilizedFurs.size() - 1;
                furIsUsed[index] = obj.dataIndex;
            }
        }
    }
    sceneState.modelPaths = utilizedMeshes;
    sceneState.furPaths = utilizedFurs;

    json data = json(sceneState);
    ofstream f(path);
    f << data.dump(4);
    f.close();
    cout << "Scene saved to " << path << "." << endl;
}

void Scene::loadFromState(const SceneState& sceneState, bool verbose){
    if ((int)sceneState.objectStates.empty()) return;

    vector<shared_ptr<Mesh>> meshes = {};
    for (const string& path: sceneState.modelPaths){
        shared_ptr<Mesh> newMesh = findMesh(path);
        if (newMesh == nullptr){
            newMesh = make_shared<Mesh>();
            newMesh->loadFromModel(path.c_str());
        }
        meshes.push_back(newMesh);
    }
    vector<shared_ptr<Fur>> furs = {};
    for (const string& path: sceneState.furPaths){
        shared_ptr<Fur> newFur = findFur(path);
        if (newFur == nullptr){
            newFur = make_shared<Fur>();
            newFur->loadFromBin(path.c_str());
        }
        furs.push_back(newFur);
    }

    deleteScene();

    m_meshes = meshes;
    m_furs = furs;
    m_objects = sceneState.objectStates;
    m_selectedObject = -1;
    m_copiedObject = -1;

    skyBottomColor = sceneState.skyBottom;
    skyMiddleColor = sceneState.skyMiddle;
    skyTopColor = sceneState.skyTop;
    skyIntensity = sceneState.skyIntensity;

    CameraProperties* camProps = m_camera->getCameraProperties();
    *camProps = sceneState.camProperties;

    updateFurs();
    updateMeshes();
    updateSceneNextFrame();
    if (verbose) cout << "Scene loaded." << endl;
}

SceneState Scene::getState(){
    SceneState sceneState;
    sceneState.objectStates = m_objects;

    sceneState.modelPaths = {};
    for (auto mesh : m_meshes){
        sceneState.modelPaths.push_back(mesh->path);
    }
    sceneState.furPaths = {};
    for (auto fur : m_furs){
        sceneState.furPaths.push_back(fur->path);
    }

    sceneState.skyBottom = skyBottomColor;
    sceneState.skyMiddle = skyMiddleColor;
    sceneState.skyTop = skyTopColor;
    sceneState.skyIntensity = skyIntensity;
    sceneState.camProperties = *(m_camera->getCameraProperties());
    return sceneState;
}


void Scene::initGPU(){
    glDeleteBuffers(1, &m_sceneBuffer);
    glGenBuffers(1, &m_sceneBuffer);
    glDeleteBuffers(1, &m_materialsBuffer);
    glGenBuffers(1, &m_materialsBuffer);
    glDeleteBuffers(1, &m_indicesBuffer);
    glGenBuffers(1, &m_indicesBuffer);
    glDeleteBuffers(1, &m_bvhInfosBuffer);
    glGenBuffers(1, &m_bvhInfosBuffer);
    glDeleteBuffers(1, &m_trianglesBuffer);
    glGenBuffers(1, &m_trianglesBuffer);
    glDeleteBuffers(1, &m_hairStrandBuffer);
    glGenBuffers(1, &m_hairStrandBuffer);
    glDeleteBuffers(1, &m_nodesBuffer);
    glGenBuffers(1, &m_nodesBuffer);
}


int Scene::addObject(Object obj){
    obj.ID = m_maxId++;
    if (obj.type == PrimType::MESH_){
        obj.name = m_meshes[obj.dataIndex]->name + string(" ") + to_string(obj.ID);
        updateMeshes();
    }
    else if (obj.type == PrimType::FUR_){
        obj.name = m_furs[obj.dataIndex]->name + string(" ") + to_string(obj.ID);
        updateFurs();
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

int Scene::addMesh(shared_ptr<Mesh> newMesh){
    m_meshes.push_back(newMesh);
    updateMeshes();
    return (int)m_meshes.size() - 1;
}

int Scene::addFur(shared_ptr<Fur> newFur){
    m_furs.push_back(newFur);
    updateFurs();
    return (int)m_furs.size() - 1;
}

void Scene::removeObject(int index){
    if (index < 0 || index >= (int)m_objects.size()) return;
    if (m_objects[index].type == PrimType::MESH_) updateMeshes();
    else if (m_objects[index].type == PrimType::FUR_) updateFurs();
    m_objects.erase(m_objects.begin() + index);
    updateScene();
    m_selectedObject = -1;
}

void Scene::removeMesh(int index){
    if (index < 0 || index >= (int)m_meshes.size()) return;
    for (Object& obj : m_objects){
        if (obj.type != PrimType::MESH_) continue;
        if (obj.dataIndex > index) obj.dataIndex -= 1;
        if (obj.dataIndex == index) obj.dataIndex = -1;
    }
    m_meshes.erase(m_meshes.begin() + index);
    m_selectedMesh = -1;
    updateMeshes();
    updateScene();
}

void Scene::removeFur(int index){
    if (index < 0 || index >= (int)m_furs.size()) return;
    for (Object& obj : m_objects){
        if (obj.type != PrimType::FUR_) continue;
        if (obj.dataIndex > index) obj.dataIndex -= 1;
        if (obj.dataIndex == index) obj.dataIndex = -1;
    }
    m_furs.erase(m_furs.begin() + index);
    m_selectedFur = -1;
    updateFurs();
    updateScene();
}

vector<const char*> Scene::getObjectNames() const {
    vector<const char*> names = {};
    for (const Object& obj : m_objects){
        names.push_back(obj.name.c_str());
    }
    return names; 
}

vector<const char*> Scene::getMeshNames() const {
    vector<const char*> result;
    for (shared_ptr<Mesh> mesh : m_meshes){
        result.push_back(mesh->name.c_str());
    }
    return result;
}

vector<const char*> Scene::getFurNames() const {
    vector<const char*> result;
    for (shared_ptr<Fur> fur : m_furs){
        result.push_back(fur->name.c_str());
    }
    return result;
}

void Scene::selectObject(int index) {
    int pIndex = 0;
    int mIndex = 0;
    int fIndex = 0;
    m_selectedObjectGPUIndex = -1;
    for (int i = 0; i < (int)m_objects.size(); i++)
    {
        const Object& obj = m_objects[i];

        if (obj.type == PrimType::MESH_){
            if (i == index){
                m_selectedObjectGPUIndex = m_numPrimObjects + mIndex;
                break;
            }
            mIndex++;
        }
        else if (obj.type == PrimType::FUR_){
            if (i == index){
                m_selectedObjectGPUIndex = m_numPrimObjects + m_numMeshObjects + fIndex;
                break;
            }
            fIndex++;
        }
        else{
            if (i == index){
                m_selectedObjectGPUIndex = pIndex;
                break;
            }
            pIndex++;
        }
    }
    
    m_selectedObject = index; 
    m_selectedMesh = -1; 
    m_selectedFur = -1; 
}

void Scene::selectMesh(int index) { 
    m_selectedMesh = index; 
    m_selectedObject = -1; 
    m_selectedFur = -1; 
    m_selectedObjectGPUIndex = -1;
}

void Scene::selectFur(int index) { 
    m_selectedFur = index; 
    m_selectedObject = -1; 
    m_selectedMesh = -1; 
    m_selectedObjectGPUIndex = -1;
}



shared_ptr<Mesh> Scene::findMesh(const string& path){
    for (shared_ptr<Mesh> mesh : m_meshes){
        if (mesh->path == path) return mesh;
    }
    return nullptr;
}

shared_ptr<Fur> Scene::findFur(const string& path){
    for (shared_ptr<Fur> fur : m_furs){
        if (fur->path == path) return fur;
    }
    return nullptr;
}

shared_ptr<Object> Scene::getObjectFromId(SceneState sceneState, unsigned int ID){
    for (const Object& obj : sceneState.objectStates){
        if (obj.ID == ID) return make_shared<Object>(obj);
    }
    return nullptr; 
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
                if (obj.dataIndex < 0 || obj.dataIndex >= (int)m_meshes.size()) break;
                shared_ptr<Mesh> mesh = m_meshes[obj.dataIndex];
                dist = Intersections::intersectMesh(ray, mesh, obj.pos, obj.scale, obj.rotation);
                break;
            }
            case PrimType::FUR_: {
                if (obj.dataIndex < 0 || obj.dataIndex >= (int)m_furs.size()) break;
                shared_ptr<Fur> fur = m_furs[obj.dataIndex];
                dist = Intersections::intersectFur(ray, fur, obj.pos, obj.scale, obj.rotation);
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

Object* Scene::getObject(int index){
    if (index < 0 || index >= (int)m_objects.size()) return nullptr;

    return &m_objects[index];
}

void Scene::copyObject(int index){
    m_copiedObject = index;
}

int Scene::pasteObject(){
    Object* newObj = getObject(m_copiedObject);
    if (newObj == nullptr) return -1;
    return addObject(*newObj);
}


void Scene::updateMeshes(){
    m_numMeshesChanged = true;
}

void Scene::updateFurs(){
    m_numFursChanged = true;
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
    glUniform1i(ShaderProgram::getVarLoc("selectedObject"), m_selectedObjectGPUIndex);

    bool sceneChanged = m_sceneChanged;
    m_sceneChanged = false;
    if (m_updateNextFrame){
        m_updateNextFrame = false;
        m_sceneChanged = true;
    }
    if (!sceneChanged) return;

    m_resetFrame();

    if (m_numMeshesChanged){
        vector<bool> meshIsUsed = getSourceUsed<Mesh>(m_meshes, PrimType::MESH_);
        m_meshesResult = {};
        cleanBVHData<Mesh, Triangle>(
            m_meshes, meshIsUsed,
            [](shared_ptr<Mesh> m) -> const vector<Triangle>& { return m->getTriangles(); },
            [](shared_ptr<Mesh> m) -> const vector<linBVHNode>& { return m->getLinNodes(); },
            m_meshesResult
        );
    }
    if (m_numFursChanged){
        vector<bool> furIsUsed = getSourceUsed<Fur>(m_furs, PrimType::FUR_);
        m_fursResult = {};
        cleanBVHData<Fur, HairPoint>(
            m_furs, furIsUsed,
            [](shared_ptr<Fur> f) -> const vector<HairPoint>& { return f->getPoints(); },
            [](shared_ptr<Fur> f) -> const vector<linBVHNode>& { return f->getLinNodes(); },
            m_fursResult
        );
    }
    if (m_numFursChanged || m_numMeshesChanged)
        m_nodes = Utils::concat<linBVHNode>({m_meshesResult.nodes, m_fursResult.nodes});

    vector<PrimitiveObject> primitives = {};
    vector<BVHInfos> meshInfos = {};
    vector<BVHInfos> furInfos = {};
    vector<int> volumeIndicies = {};
    int numVolumeMeshes = 0; 
    int numVolumePrims = 0;
    vector<Material> materials = {};
    vector<int> lightIndicies = {};
    for (const Object& obj : m_objects){
        int matIndex = addMaterial(materials, obj.mat);
        if (obj.type == PrimType::MESH_){
            if (obj.dataIndex < 0 || obj.dataIndex >= (int)m_meshes.size()) continue;
            if (obj.mat.type == MatType::GLASS && (obj.mat.data.w > 0)){
                volumeIndicies.insert(volumeIndicies.begin() + numVolumeMeshes, (int)meshInfos.size());
                numVolumeMeshes++;
            }
            BVHInfos meshInfo;
            meshInfo.leafOffset = m_meshesResult.leavesOffsets[obj.dataIndex];
            meshInfo.nodeOffset = m_meshesResult.nodeOffsets[obj.dataIndex];
            meshInfo.numberOfNodes = m_meshesResult.numberOfNodes[obj.dataIndex];
            meshInfo.pos = obj.pos;
            meshInfo.scale = obj.scale;
            meshInfo.rotation = obj.rotation;
            meshInfo.matIndex = matIndex;
            meshInfo.data.x = obj.data;
            meshInfos.push_back(meshInfo);
        }
        else if (obj.type == PrimType::FUR_){
            if (obj.dataIndex < 0 || obj.dataIndex >= (int)m_furs.size()) continue;
            
            BVHInfos furInfo;
            furInfo.leafOffset = m_fursResult.leavesOffsets[obj.dataIndex];
            furInfo.nodeOffset = (int)m_meshesResult.nodes.size() + m_fursResult.nodeOffsets[obj.dataIndex];
            furInfo.numberOfNodes = m_fursResult.numberOfNodes[obj.dataIndex];
            furInfo.pos = obj.pos;
            furInfo.scale = obj.scale;
            furInfo.rotation = obj.rotation;
            furInfo.matIndex = matIndex;
            furInfo.data.x = obj.data;
            furInfos.push_back(furInfo);
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
    m_numMeshObjects = (int)meshInfos.size();
    m_numFurObjects = (int)furInfos.size();
    m_numPrimObjects = (int)primitives.size();
    vector<BVHInfos> bvhInfos = Utils::concat<BVHInfos>({meshInfos, furInfos});
    vector<int> indices = Utils::concat<int>({lightIndicies, volumeIndicies});

    glUniform1i(ShaderProgram::getVarLoc("numPrimitives"), m_numPrimObjects);

    glUniform1i(ShaderProgram::getVarLoc("numLights"), (int)lightIndicies.size());
    glUniform1i(ShaderProgram::getVarLoc("numVolumesMeshes"), numVolumeMeshes);
    glUniform1i(ShaderProgram::getVarLoc("numVolumesPrims"), numVolumePrims);

    if (m_numMeshesChanged){
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_trianglesBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, (int)m_meshesResult.leaves.size() * sizeof(Triangle), m_meshesResult.leaves.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_trianglesBuffer);
    }
    if (m_numFursChanged){
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_hairStrandBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, (int)m_fursResult.leaves.size() * sizeof(HairPoint), m_fursResult.leaves.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, m_hairStrandBuffer);
    }
    if (m_numFursChanged || m_numMeshesChanged){
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_nodesBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, (int)m_nodes.size() * sizeof(linBVHNode), m_nodes.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_nodesBuffer);
        glUniform1i(ShaderProgram::getVarLoc("numBVHNodes"), (int)m_nodes.size());
        m_numMeshesChanged = false;
        m_numFursChanged = false;
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_sceneBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, (int)primitives.size() * sizeof(PrimitiveObject), primitives.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_sceneBuffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_indicesBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, (int)indices.size() * sizeof(int), indices.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_indicesBuffer);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_bvhInfosBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, (int)bvhInfos.size() * sizeof(BVHInfos), bvhInfos.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_bvhInfosBuffer);
    glUniform1i(ShaderProgram::getVarLoc("numMeshes"), m_numMeshObjects);
    glUniform1i(ShaderProgram::getVarLoc("numHair"), m_numFurObjects);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_materialsBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, (int)materials.size() * sizeof(Material), materials.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, m_materialsBuffer);

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
    m_furs.clear();
}