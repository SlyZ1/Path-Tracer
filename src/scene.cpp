#include "scene.hpp"
#include <cfloat>

using namespace glm;

glm::vec3 Scene::m_cameraDirection = glm::vec3(0, 0, -1);
glm::vec3 Scene::m_cameraPosition  = glm::vec3(0, 0, 0);

Scene Scene::defaultScene(shared_ptr<App> app, shared_ptr<Camera> camera, function<void()> resetFrame){
    Scene scene = Scene(app, camera, resetFrame);
    scene.initGPU();

    Object light;
    light.type = PrimType::SPHERE;
    light.pos = vec3(0, 5, 2);
    light.scale = 1;
    light.mat = Material::emitMaterial(vec3(1), 20);
    scene.addObject(light);

    Object plane;
    plane.type = PrimType::PLANE;
    plane.pos = vec3(0);
    plane.scale = 1;
    plane.mat = Material::glossyMaterial(vec3(1), 0, 0.2);
    scene.addObject(plane);

    Object meshObject;
    meshObject.pos = vec3(0, 1, 0);
    meshObject.scale = 3;
    meshObject.type = PrimType::MESH_;
    meshObject.mat = Material::glossyMaterial(vec3(0.9f, 0.6f, 0.2f), 0, 0.3f);
    meshObject.mesh = new Mesh();
    meshObject.mesh->loadFromModel("models/dragon.obj");
    scene.addObject(meshObject);

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
    glDeleteBuffers(1, &m_meshInfosBuffer);
    glGenBuffers(1, &m_meshInfosBuffer);
    glDeleteBuffers(1, &m_trianglesBuffer);
    glGenBuffers(1, &m_trianglesBuffer);
    glDeleteBuffers(1, &m_nodesBuffer);
    glGenBuffers(1, &m_nodesBuffer);
}

float Scene::intersectSphere(const Ray& ray, const Object& sphere){
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

float Scene::intersectPlane(const Ray& ray, const Object& plane){
    vec3 rp = plane.pos - ray.origin;
    vec3 normal = vec3(0,1,0);
    float t = dot(rp, normal) / dot(ray.direction, normal);
    return t;
}

float Scene::intersectAABB(const Ray& invRay, const AABB& aabb, float tMin, float tMax){
    vec3 t0 = (aabb.min - invRay.origin) * invRay.direction;
    vec3 t1 = (aabb.max - invRay.origin) * invRay.direction;

    vec3 tNear = min(t0, t1);
    vec3 tFar  = max(t0, t1);

    float tmin = glm::max(glm::max(tNear.x, tNear.y), glm::max(tNear.z, tMin));
    float tmax = glm::min(glm::min(tFar.x,  tFar.y),  glm::min(tFar.z,  tMax));

    if (tmax < tmin) return -1;
    return tmin;
}

float Scene::intersectTriangle(const Ray& ray, const Triangle& triangle){
    vec3 edge1 = triangle.v2 - triangle.v1;
    vec3 edge2 = triangle.v3 - triangle.v1;

    vec3 pvec = cross(ray.direction, edge2);
    float det = dot(edge1, pvec);

    if (abs(det) < 1e-5)
        return -1; // rayon parallèle

    float invDet = 1.0 / det;
    vec3 tvec = ray.origin - triangle.v1;
    float u = dot(tvec, pvec) * invDet;
    if (u < 0.0 || u > 1.0)
        return -1;

    vec3 qvec = cross(tvec, edge1);
    float v = dot(ray.direction, qvec) * invDet;
    if (v < 0.0 || u + v > 1.0)
        return -1;

    float t = dot(edge2, qvec) * invDet;
    if (t < 0.001) return -1;
    return t;
}

float Scene::intersectMesh(const Ray& ray, const Object& obj){
    const int STACK_SIZE = 32;

    int stack[STACK_SIZE];
    int stackPtr = 0;

    Mesh mesh = *obj.mesh;
    const vector<linBVHNode>& nodes = mesh.getLinNodes();
    const vector<Triangle>& triangles = mesh.getTriangles();
    stack[stackPtr++] = nodes.size() - 1;

    vec3 pos = obj.pos;
    float scale = obj.scale;

    float hitT = 1e6;
    Ray newRay = ray;
    newRay.origin -= pos;
    Ray invRay = newRay;
    invRay.direction = 1.0f / invRay.direction;

    while (stackPtr > 0) {
        int nodeIndex = stack[--stackPtr];
        linBVHNode node = nodes[nodeIndex];

        float boxDist = intersectAABB(invRay, Mesh::scaleAABB(node.bounds, scale), 0.001f, hitT);
        if (boxDist < 0 || boxDist > hitT) continue;

        if (node.triangle >= 0) {
            float triDist = intersectTriangle(newRay, Mesh::scaleTri(triangles[node.triangle], scale));
            if (triDist >= 0) {
                if (triDist < hitT) {
                    hitT = triDist;
                }
            }
        }
        else {
            float leftDist = -1;
            float rightDist = -1;
            if (node.left >= 0)
                leftDist = intersectAABB(invRay, Mesh::scaleAABB(nodes[node.left].bounds, scale), 0.001f, hitT);
            if (node.right >= 0)
                rightDist = intersectAABB(invRay, Mesh::scaleAABB(nodes[node.right].bounds, scale), 0.001f, hitT);

            if (leftDist >= 0 && rightDist >= 0){
                if (leftDist < rightDist) {
                    stack[stackPtr++] = node.right;
                    stack[stackPtr++] = node.left;
                } else {
                    stack[stackPtr++] = node.left;
                    stack[stackPtr++] = node.right;
                }
            } else if (leftDist >= 0) stack[stackPtr++] = node.left;
            else if (rightDist >= 0) stack[stackPtr++] = node.right;
        }
    }

    //ray.origin += modelPos;
    return hitT;
}


int Scene::intersectObject(const Ray& ray){
    float distance = FLT_MAX;
    int intersected = -1;
    for(size_t i = 0; i < m_objects.size(); i++){
        float dist = -1;
        Object obj = m_objects[i];
        switch(obj.type){
            case PrimType::SPHERE:
                dist = intersectSphere(ray, obj);
                break;
            case PrimType::PLANE:
                dist = intersectPlane(ray, obj);
                break;
            case PrimType::MESH_:
                dist = intersectMesh(ray, obj);
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

int Scene::addObject(const Object& obj){
    int newIndex = m_objects.size();
    if (obj.type == PrimType::MESH_) m_numMeshesChanged = true;
    else if (obj.mat.type == MatType::EMIT) m_lightIndices.push_back(newIndex);
    m_objects.push_back(obj);
    m_sceneChanged = true;
    m_selectedObject = newIndex;
    return newIndex;
}

Object* Scene::getObject(int index){
    if (index < 0 || index > (int)m_objects.size()) return nullptr;

    return &m_objects[index];
}

void Scene::removeObject(int index){
    if (index < 0 || index >= (int)m_objects.size()) return;

    Object obj = m_objects[index];
    if (obj.type == PrimType::MESH_) m_numMeshesChanged = true;
    else if (obj.mat.type == MatType::EMIT) 
        m_lightIndices.erase(
            std::remove(m_lightIndices.begin(), m_lightIndices.end(), index),
            m_lightIndices.end()
        );
    m_objects.erase(m_objects.begin() + index);
    m_sceneChanged = true;
}

void Scene::copyObject(int index){
    m_copiedObject = index;
}

int Scene::pasteObject(){
    Object newObj = *getObject(m_copiedObject);
    return addObject(newObj);
}

void Scene::updateScene(){
    m_sceneChanged = true;
}

void Scene::updateGPU(){
    if (!m_sceneChanged) return;
    m_sceneChanged = false;
    m_resetFrame();

    vector<PrimitiveObject> primitives;
    vector<MeshInfos> meshInfos;
    vector<Triangle> triangles;
    vector<linBVHNode> nodes;
    for(Object obj : m_objects){
        if (obj.type == PrimType::MESH_){
            MeshInfos meshInfo;
            meshInfo.triangleOffset = triangles.size();
            meshInfo.nodeOffset = nodes.size();
            meshInfo.pos = obj.pos;
            meshInfo.scale = obj.scale;
            meshInfo.mat = obj.mat;
            meshInfo.isSmooth = obj.mesh->isSmooth;
            meshInfos.push_back(meshInfo);
            if (obj.mat.type == MatType::EMIT) obj.mat.type = MatType::DIFFUSE;
            
            const vector<Triangle>& meshTriangles = obj.mesh->getTriangles();
            const vector<linBVHNode>& meshNodes = obj.mesh->getLinNodes();
            triangles.insert(triangles.end(), meshTriangles.begin(), meshTriangles.end());
            nodes.insert(nodes.end(), meshNodes.begin(), meshNodes.end());
        }
        else{
            PrimitiveObject prim;
            prim.pos = obj.pos;
            prim.scale = obj.scale;
            prim.mat = obj.mat;
            prim.type = obj.type;
            primitives.push_back(prim);
        }
    }

    glUniform1i(ShaderProgram::getVarLoc("numPrimitives"), primitives.size());
    glUniform1i(ShaderProgram::getVarLoc("numLights"), m_lightIndices.size());

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_sceneBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, primitives.size() * sizeof(PrimitiveObject), primitives.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_sceneBuffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_lightIndicesBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_lightIndices.size() * sizeof(int), m_lightIndices.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_lightIndicesBuffer);

    if (m_numMeshesChanged){
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_trianglesBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, triangles.size() * sizeof(Triangle), triangles.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_trianglesBuffer);
        glUniform1i(ShaderProgram::getVarLoc("numTriangles"), triangles.size());
    
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_nodesBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, nodes.size() * sizeof(linBVHNode), nodes.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_nodesBuffer);
        glUniform1i(ShaderProgram::getVarLoc("numBVHNodes"), nodes.size());
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_meshInfosBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, meshInfos.size() * sizeof(MeshInfos), meshInfos.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_meshInfosBuffer);
    glUniform1i(ShaderProgram::getVarLoc("numMeshes"), meshInfos.size());
    
}