#include "scene.hpp"
#include <cfloat>

using namespace glm;

glm::vec3 Scene::m_cameraDirection = glm::vec3(0, 0, -1);
glm::vec3 Scene::m_cameraPosition  = glm::vec3(0, 0, 0);

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

Scene Scene::defaultScene(shared_ptr<App> app, shared_ptr<Camera> camera, function<void()> resetFrame){
    Scene scene = Scene(app, camera, resetFrame);
    scene.initGPU();

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

Ray Scene::rayFromClick(glm::vec3 origin, glm::vec3 dir, glm::vec2 screenPos){
    Ray ray;
    ray.origin = origin;
    m_cameraDirection = normalize(dir);
    m_cameraPosition = origin;

    float fov = radians(50.0);
    vec3 forward = m_cameraDirection;

    vec3 worldUp = abs(forward.y) < 0.999
                 ? vec3(0,1,0)
                 : vec3(0,0,1);

    vec3 right = normalize(cross(forward, worldUp));
    vec3 up = cross(right, forward);

    float tanHalfFov = tan(fov * 0.5);

    vec3 newDir = forward + (right * screenPos.x + up * screenPos.y) * tanHalfFov;
    ray.direction = normalize(newDir);
    return ray;
}

glm::vec2 Scene::worldToScreen(glm::vec3 worldPos){
    vec3 forward = m_cameraDirection;

    vec3 worldUp = abs(forward.y) < 0.999
                 ? vec3(0,1,0)
                 : vec3(0,0,1);

    vec3 right = normalize(cross(forward, worldUp));
    vec3 up = cross(right, forward);

    vec3 dir = normalize(worldPos - m_cameraPosition);
    float normFactor = 1 / dot(forward, dir);
    dir *= normFactor;
    float fov = m_camera->getCameraProperties()->fov;
    float tanHalfFov = tan(radians(fov) * 0.5);
    float rightCompo = dot(right, dir) / tanHalfFov;
    float upCompo = -dot(up, dir) / tanHalfFov;
    return vec2(rightCompo * m_app->height() / 2.0f, upCompo * m_app->height() / 2.0f);
}

void Scene::initGPU(){
    glDeleteBuffers(1, &m_sceneBuffer);
    glGenBuffers(1, &m_sceneBuffer);
    glDeleteBuffers(1, &m_lightIndicesBuffer);
    glGenBuffers(1, &m_lightIndicesBuffer);
}

float Scene::intersectSphere(const Ray& ray, const Primitive& sphere){
    vec3 oc = ray.origin - sphere.pos;
    float b = dot(oc, ray.direction);
    float c = dot(oc, oc) - sphere.scale * sphere.scale;
    float h = b*b - c;

    float distance = -1;
    if (h < 0) return distance;

    float sqrtH = sqrt(h);
    distance = -b - sqrtH;
    if (distance <= 0){
        distance = -b + sqrtH;
    }

    return distance;
}

float Scene::intersectPlane(const Ray& ray, const Primitive& plane){
    vec3 rp = plane.pos - ray.origin;
    vec3 normal = vec3(0,1,0);
    float t = dot(rp, normal) / dot(ray.direction, normal);
    return t;
}


int Scene::intersectObject(const Ray& ray){
    float distance = FLT_MAX;
    int intersected = -1;
    for(size_t i = 0; i < m_prims.size(); i++){
        float dist = -1;
        Primitive prim = m_prims[i];
        switch(prim.type){
            case PrimType::SPHERE:
                dist = intersectSphere(ray, prim);
                break;
            case PrimType::PLANE:
                dist = intersectPlane(ray, prim);
                break;
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

int Scene::addObject(const Primitive& prim){
    int newIndex = m_prims.size();
    if (prim.mat.type == MatType::EMIT) m_lightIndices.push_back(newIndex);
    m_prims.push_back(prim);
    m_sceneChanged = true;
    return newIndex;
}

Primitive* Scene::getObject(int index){
    if (index < 0 || index > (int)m_prims.size()) return nullptr;

    return &m_prims[index];
}

void Scene::removeObject(int index){
    if (index < 0 || index >= (int)m_prims.size()) return;

    Primitive prim = m_prims[index];
    if (prim.mat.type == MatType::EMIT) 
        m_lightIndices.erase(
            std::remove(m_lightIndices.begin(), m_lightIndices.end(), index),
            m_lightIndices.end()
        );
    m_prims.erase(m_prims.begin() + index);
    m_sceneChanged = true;
}

void Scene::copyPrimitive(int index){
    m_copiedPrimitive = index;
}

int Scene::pastePrimitive(){
    Primitive newPrim = *getObject(m_copiedPrimitive);
    return addObject(newPrim);
}

void Scene::updateScene(){
    m_sceneChanged = true;
}

void Scene::updateGPU(){
    if (!m_sceneChanged) return;

    m_resetFrame();

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