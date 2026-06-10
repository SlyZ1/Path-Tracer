#ifndef SCENE
#define SCENE

#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include "camera.hpp"
#include "app.hpp"
#include "shader_program.hpp"
#include "mesh.hpp"
#include "material.hpp"

using namespace std;

enum PrimType : int {
    SPHERE = 0,
    PLANE = 1,
    CUBE = 2,
    MESH_ = 3
};

struct PrimitiveObject {
    glm::vec3 pos;
    float scale;
    Material mat;
    glm::vec3 pad;
    PrimType type;
};

struct MeshInfos {
    glm::vec3 pos;
    float scale;
    int triangleOffset;
    int nodeOffset;
    int isSmooth; 
    int pad;
    Material mat;
};

struct Object {
    glm::vec3 pos;
    float scale;
    Material mat;
    glm::vec3 pad;
    PrimType type;
    Mesh* mesh = nullptr;
};

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

class Scene {
private:
    shared_ptr<App> m_app = {};
    shared_ptr<Camera> m_camera = {};

    GLuint m_sceneBuffer = 0;
    GLuint m_lightIndicesBuffer = 0;
    GLuint m_meshInfosBuffer = 0;
    GLuint m_trianglesBuffer = 0;
    GLuint m_nodesBuffer = 0;
    bool m_sceneChanged = false;
    bool m_numMeshesChanged = false;
    vector<Object> m_objects = {};
    vector<int> m_lightIndices = {};
    static glm::vec3 m_cameraDirection;
    static glm::vec3 m_cameraPosition;

    int m_copiedObject = -1;
    int m_selectedObject = -1;
    
    function<void()> m_resetFrame = {};
    
    float intersectSphere(const Ray& ray, const Object& sphere);
    float intersectPlane(const Ray& ray, const Object& plane);
    float intersectAABB(const Ray& ray, const AABB& aabb, float tMin, float tMax);
    float intersectTriangle(const Ray& ray, const Triangle& triangle);
    float intersectMesh(const Ray& ray, const Object& obj);

public:
    Scene() = default;
    Scene(shared_ptr<App> app, shared_ptr<Camera> camera, function<void()> resetFrame) 
    : m_app(app), m_camera(camera), m_resetFrame(resetFrame) {}
    static Scene defaultScene(shared_ptr<App> app, shared_ptr<Camera> camera, function<void()> resetFrame = nullptr);
    static Ray rayFromClick(shared_ptr<Camera> camera, glm::vec2 screenPos);
    glm::vec2 worldToScreen(glm::vec3 worldPos);
    void initGPU();
    int intersectObject(const Ray& ray);
    int addObject(const Object& prim);
    Object* getObject(int index);
    void removeObject(int index);
    void copyObject(int index);
    int pasteObject();
    void selectObject(int index) { m_selectedObject = index; };
    int getSelectedObject() const {return m_selectedObject; };
    void updateScene();
    void updateGPU();
};
#endif