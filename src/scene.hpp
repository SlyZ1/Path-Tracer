#ifndef SCENE
#define SCENE

#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include "camera.hpp"
#include "app.hpp"
#include "shader_program.hpp"

using namespace std;

enum MatType : int {
    DIFFUSE = 0,
    METAL = 1,
    GLASS = 2,
    GLOSSY = 3,
    EMIT = 4
};

struct Material {
    glm::vec3 color;
    MatType type;
    glm::vec2 data;
    glm::vec2 pad;
};

enum PrimType : int {
    SPHERE = 0,
    PLANE = 1,
    CUBE = 2,
};

struct Primitive {
    glm::vec3 pos;
    float scale;
    Material mat;
    glm::vec3 pad;
    PrimType type;
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
    bool m_sceneChanged = false;
    vector<Primitive> m_prims = {};
    vector<int> m_lightIndices = {};
    static glm::vec3 m_cameraDirection;
    static glm::vec3 m_cameraPosition;

    int m_copiedPrimitive = -1;
    int m_selectedPrimitive = -1;
    
    function<void()> m_resetFrame = {};
    
    float intersectSphere(const Ray& ray, const Primitive& sphere);
    float intersectPlane(const Ray& ray, const Primitive& plane);

public:
    Scene() = default;
    Scene(shared_ptr<App> app, shared_ptr<Camera> camera, function<void()> resetFrame) 
    : m_app(app), m_camera(camera), m_resetFrame(resetFrame) {}
    static Scene defaultScene(shared_ptr<App> app, shared_ptr<Camera> camera, function<void()> resetFrame = nullptr);
    static Material diffuseMaterial(glm::vec3 color, float roughness);
    static Material metalMaterial(glm::vec3 color, float fuzziness);
    static Material glassMaterial(glm::vec3 color, float refractionIndex);
    static Material glossyMaterial(glm::vec3 color, float fuzziness, float metallic);
    static Material emitMaterial(glm::vec3 color, float intensity);
    static Ray rayFromClick(glm::vec3 origin, glm::vec3 dir, glm::vec2 screenPos);
    glm::vec2 worldToScreen(glm::vec3 worldPos);
    void initGPU();
    int intersectObject(const Ray& ray);
    int addObject(const Primitive& prim);
    Primitive* getObject(int index);
    void removeObject(int index);
    void copyPrimitive(int index);
    int pastePrimitive();
    void selectPrimitive(int index) { m_selectedPrimitive = index; };
    int getSelectedPrimitive() const {return m_selectedPrimitive; };
    void updateScene();
    void updateGPU();
};
#endif