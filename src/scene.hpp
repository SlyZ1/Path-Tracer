#ifndef SCENE
#define SCENE

#include <glm/glm.hpp>
#include <vector>
#include "shader_program.hpp"

using namespace std;

enum MatType : int {
    DIFFUSE = 0,
    METAL = 1,
    GLASS = 2,
    GLOSSY = 3
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

class Scene {
private:
    GLuint sceneBuffer = 0;
    bool m_sceneChanged = false;
    vector<Primitive> m_prims = {};

public:
    Scene() = default;
    static Material diffuseMaterial(glm::vec3 color, float roughness);
    static Material metalMaterial(glm::vec3 color, float fuzziness);
    static Material glassMaterial(glm::vec3 color, float refractionIndex);
    static Material glossyMaterial(glm::vec3 color, float fuzziness, float metallic);
    static Scene defaultScene();
    void initGPU();
    void addObject(const Primitive& prim);
    void updateGPU();
};
#endif