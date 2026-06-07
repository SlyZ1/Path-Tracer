#include "scene.hpp"

using namespace glm;

Material Scene::diffuseMaterial(vec3 color, float roughness){
    Material mat;
    mat.type = MatType::DIFFUSE;
    mat.color = color;
    mat.data = vec2(glm::clamp(roughness, 0.0f, 1.0f), 0);
    return mat;
}

Material Scene::metalMaterial(vec3 color, float fuzziness){
    Material mat;
    mat.type = MatType::METAL;
    mat.color = color;
    mat.data = vec2(glm::clamp(fuzziness, 0.0f, 1.0f), 0);
    return mat;
}

Material Scene::glassMaterial(vec3 color, float refractionIndex){
    Material mat;
    mat.type = MatType::GLASS;
    mat.color = color;
    mat.data = vec2(0, glm::max(refractionIndex, 0.0f));
    return mat;
}

Material Scene::glossyMaterial(vec3 color, float fuzziness, float metallic){
    Material mat;
    mat.type = MatType::GLOSSY;
    mat.color = color;
    mat.data = vec2(glm::clamp(fuzziness, 0.0f, 1.0f), glm::clamp(metallic, 0.0f, 1.0f));
    return mat;
}

Material Scene::emitMaterial(vec3 color, float intensity){
    Material mat;
    mat.type = MatType::EMIT;
    mat.color = color;
    mat.data = vec2(glm::max(intensity, 0.0f), 0);
    return mat;
}

Scene Scene::defaultScene(){
    Scene scene = Scene();
    scene.initGPU();

    // Primitive sphere;
    // sphere.type = PrimType::SPHERE;
    // sphere.pos = vec3(0, 1, 0);
    // sphere.scale = 1;
    // sphere.mat = glassMaterial(vec3(1), 2);
    // scene.addObject(sphere);

    Primitive sphere2;
    sphere2.type = PrimType::SPHERE;
    sphere2.pos = vec3(-2, 1, 0);
    sphere2.scale = 1;
    sphere2.mat = metalMaterial(vec3(1), 0.3);
    scene.addObject(sphere2);

    Primitive light;
    light.type = PrimType::SPHERE;
    light.pos = vec3(-2, 3, -5);
    light.scale = 1;
    light.mat = emitMaterial(vec3(1), 10);
    scene.addObject(light);

    Primitive plane;
    plane.type = PrimType::PLANE;
    plane.pos = vec3(0);
    plane.scale = 1;
    plane.mat = glossyMaterial(vec3(1), 0, 0.2);
    scene.addObject(plane);

    return scene;
}

void Scene::initGPU(){
    glDeleteBuffers(1, &m_sceneBuffer);
    glGenBuffers(1, &m_sceneBuffer);
    glDeleteBuffers(1, &m_lightIndicesBuffer);
    glGenBuffers(1, &m_lightIndicesBuffer);
}

void Scene::addObject(const Primitive& prim){
    if (prim.mat.type == MatType::EMIT) m_lightIndices.push_back(m_prims.size());
    m_prims.push_back(prim);
    m_sceneChanged = true;
}

void Scene::updateGPU(){
    if (!m_sceneChanged) return;

    glUniform1i(ShaderProgram::getVarLoc("numPrimitives"), m_prims.size());
    glUniform1i(ShaderProgram::getVarLoc("numLights"), m_lightIndices.size());

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_sceneBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_prims.size() * sizeof(Primitive), m_prims.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_sceneBuffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_lightIndicesBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_lightIndices.size() * sizeof(int), m_lightIndices.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_lightIndicesBuffer);

    m_sceneChanged = false;
}