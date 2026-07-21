#ifndef SCENE
#define SCENE

#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include <nlohmannjson/json.hpp>
namespace glm {
    inline void to_json(nlohmann::json& j, const vec2& v) { j = {v.x, v.y}; }
    inline void from_json(const nlohmann::json& j, vec2& v) { v.x = j[0]; v.y = j[1]; }

    inline void to_json(nlohmann::json& j, const vec3& v) { j = {v.x, v.y, v.z}; }
    inline void from_json(const nlohmann::json& j, vec3& v) { v.x = j[0]; v.y = j[1]; v.z = j[2]; }

    inline void to_json(nlohmann::json& j, const vec4& v) { j = {v.x, v.y, v.z, v.w}; }
    inline void from_json(const nlohmann::json& j, vec4& v) { v.x = j[0]; v.y = j[1]; v.z = j[2]; v.w = j[3]; }
}
#include "camera.hpp"
#include "app.hpp"
#include "shader_program.hpp"
#include "mesh.hpp"
#include "material.hpp"
#include "intersections.hpp"

using namespace std;
using json = nlohmann::json;

struct PrimitiveObject {
    vec3 pos;
    int matIndex = -1;
    vec3 scale;
    PrimType type;
    vec3 rotation;
    int pad;
};

struct MeshInfos {
    vec3 pos;
    int triangleOffset;
    vec3 scale;
    int nodeOffset;
    vec3 rotation;
    int numberOfNodes;
    int isSmooth;
    int matIndex = -1;
    vec2 pad;
};

struct Object {
    string name;
    vec3 pos;
    vec3 scale = vec3(1.0);
    vec3 rotation;
    Material mat = Material();
    unsigned int ID;
    PrimType type;
    int meshIndex;
    bool isSmooth;

    Object operator+(const Object& other){
        Object newObj;
        newObj.name = name;
        newObj.pos = pos + other.pos;
        newObj.scale = scale + other.scale;
        newObj.rotation = mod(mod(rotation + other.rotation, 360.0f) + 360.0f, 360.0f);
        newObj.mat = mat + other.mat;
        newObj.ID = ID;
        newObj.type = type;
        newObj.meshIndex = meshIndex;
        newObj.isSmooth = isSmooth;
        return newObj;
    }

    Object operator*(float t){
        Object newObj;
        newObj.name = name;
        newObj.pos = pos * t;
        newObj.scale = scale * t;
        newObj.rotation = mod(mod(rotation * t, 360.0f) + 360.0f, 360.0f);
        newObj.mat = mat * t;
        newObj.ID = ID;
        newObj.type = type;
        newObj.meshIndex = meshIndex;
        newObj.isSmooth = isSmooth;
        return newObj;
    }
};

struct SceneState {
    float skyIntensity;
    vec3 skyTop;
    vec3 skyMiddle;
    vec3 skyBottom;
    CameraProperties camProperties;
    vector<string> modelPaths;
    vector<Object> objectStates;
};

NLOHMANN_JSON_SERIALIZE_ENUM(MatType, {
    { MatType::DIFFUSE,     "diffuse"   },
    { MatType::METAL,       "metal"     },
    { MatType::GLASS,       "glass"     },
    { MatType::GLOSSY,      "glossy"    },
    { MatType::EMIT,        "emit"      }
})
NLOHMANN_JSON_SERIALIZE_ENUM(PrimType, {
    { PrimType::SPHERE,     "sphere"    },
    { PrimType::PLANE,      "plane"     },
    { PrimType::CUBE,       "cube"      },
    { PrimType::CYLINDER,   "cylinder"  },
    { PrimType::MESH_,      "mesh"      }
})
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CameraProperties, fov, aperture, focalLength)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Material, color, color2, type, data, data2)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Object, name, pos, scale, rotation, mat, ID, type, meshIndex, isSmooth)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SceneState, skyIntensity, skyTop, skyMiddle, skyBottom, camProperties, modelPaths, objectStates)

class Scene {
private:
    shared_ptr<App> m_app = {};
    shared_ptr<Camera> m_camera = {};

    GLuint m_sceneBuffer = 0;
    GLuint m_materialsBuffer = 0;
    GLuint m_lightIndicesBuffer = 0;
    GLuint m_volumeIndicesBuffer = 0;
    GLuint m_meshInfosBuffer = 0;
    GLuint m_trianglesBuffer = 0;
    GLuint m_nodesBuffer = 0;
    bool m_sceneChanged = false;
    bool m_updateNextFrame = false;
    bool m_numMeshesChanged = false;
    vector<Object> m_objects = {};
    vector<shared_ptr<Mesh>> m_meshes = {};
    static vec3 m_cameraDirection;
    static vec3 m_cameraPosition;

    int m_copiedObject = -1;
    int m_selectedObject = -1;
    int m_selectedMesh = -1;
    unsigned int m_maxId = 0;

    bool m_spectral = false;
    
    function<void()> m_resetFrame = {};

    int addMaterial(vector<Material>& materials, Material mat);

    shared_ptr<Mesh> findMesh(const string& path);

public:
    Scene() = default;
    Scene(shared_ptr<App> app, shared_ptr<Camera> camera, function<void()> resetFrame) 
    : m_app(app), m_camera(camera), m_resetFrame(resetFrame) {}
    static shared_ptr<Scene> defaultScene(shared_ptr<App> app, shared_ptr<Camera> camera, function<void()> resetFrame = nullptr);
    static Ray rayFromClick(shared_ptr<Camera> camera, vec2 screenPos);
    static SceneState stateFromJson(const string& path);
    static void stateToJson(SceneState sceneState, const string& path);
    static shared_ptr<Object> getObjectFromId(SceneState sceneState, unsigned int ID);

    static const char* primLabels[5];

    float skyIntensity = 0.3f;
    glm::vec3 skyTopColor = vec3(0.32f, 0.55f, 0.78f);
    glm::vec3 skyMiddleColor = vec3(0.75f, 0.78f, 0.82f);
    glm::vec3 skyBottomColor = vec3(1.00f, 0.65f, 0.30f);

    void setSpectral(bool spectral) { m_spectral = spectral; }
    bool getSpectral() const { return m_spectral; }
    void loadFromState(const SceneState& sceneState, bool verbose = true);
    vector<const char*> getObjectNames() const;
    SceneState getState();
    vec2 worldToScreen(shared_ptr<Camera> camera, vec3 worldPos);
    void initGPU();
    int intersectObject(const Ray& ray);
    int addMesh(shared_ptr<Mesh> newMesh);
    void removeMesh(int index);
    int addObject(Object prim);
    Object* getObject(int index);
    void removeObject(int index);
    void copyObject(int index);
    int pasteObject();
    void selectObject(int index) { m_selectedObject = index; m_selectedMesh = -1; };
    int getSelectedObject() const { return m_selectedObject; };
    void selectMesh(int index) { m_selectedMesh = index; m_selectedObject = -1; };
    int getSelectedMesh() const { return m_selectedMesh; };
    vector<const char*> getMeshNames() const;
    void updateMeshes();
    void updateScene();
    void updateSceneNextFrame();
    void updateGPU();
    void deleteScene();
};
#endif