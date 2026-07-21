#include "intersections.hpp"

float Intersections::dot2(vec3 x){
    return dot(x, x);
}

mat3 Intersections::rotationMatrix(vec3 rotationDegrees){
    vec3 r = radians(rotationDegrees);
    float cx = cos(r.x), sx = sin(r.x);
    float cy = cos(r.y), sy = sin(r.y);
    float cz = cos(r.z), sz = sin(r.z);

    mat3 rx = transpose(mat3(
        1,  0,   0,
        0,  cx, -sx,
        0,  sx,  cx
    ));

    mat3 ry = transpose(mat3(
        cy,  0, sy,
        0,   1, 0,
        -sy, 0, cy
    ));

    mat3 rz = transpose(mat3(
        cz, -sz, 0,
        sz,  cz, 0,
        0,   0,  1
    ));

    return rz * ry * rx;
}

float Intersections::intersectSphere(const Ray& ray, vec3 pos, vec3 scale, vec3 rotation) {
    mat3 rot = rotationMatrix(rotation);
    mat3 invRot = transpose(rot);

    vec3 dir = (invRot * ray.direction) / scale;
    vec3 oc = (invRot * (ray.origin - pos)) / scale;

    float a = dot(dir, dir);
    float b = dot(oc, dir);
    float c = dot(oc, oc) - 1.0f;

    float h = b*b - a*c;

    if (h < 0.) return -1.0f;

    float sqrtH = sqrt(h);
    float t = (-b - sqrtH) / a;
    if (t <= 0){
        t = (-b + sqrtH) / a;
        if (t <= 0) return -1.0f;
    }
    return t;
}

float Intersections::intersectPlane(const Ray& ray, vec3 pos, vec3 scale, vec3 rotation) {
    mat3 rot = rotationMatrix(rotation);
    mat3 invRot = transpose(rot);

    vec3 localOrigin = invRot * (ray.origin - pos);  
    vec3 localDir = invRot * ray.direction;

    vec3 normal = vec3(0.0f, 1.0f, 0.0f);
    float t = -localOrigin.y / localDir.y;
    vec3 localHit = localOrigin + t * localDir;
    vec3 difference = abs(localHit);
    
    if (t <= 0 || difference.x > scale.x || difference.y > scale.y || difference.z > scale.z)
        return -1.0f;
    return t;
}

float Intersections::intersectCube(const Ray& ray, vec3 pos, vec3 scale, vec3 rotation) {
    mat3 rot = rotationMatrix(rotation);
    mat3 invRot = transpose(rot);

    vec3 localOrigin = invRot * (ray.origin - pos);
    vec3 localDir = invRot * ray.direction;

    vec3 minP = -scale;
    vec3 maxP = scale;

    vec3 t0 = (minP - localOrigin) / localDir;
    vec3 t1 = (maxP - localOrigin) / localDir;

    vec3 tNear = min(t0, t1);
    vec3 tFar = max(t0, t1);

    float tmin = glm::max(tNear.x, glm::max(tNear.y, tNear.z));
    float tmax = glm::min(tFar.x,  glm::min(tFar.y,  tFar.z));

    if (tmax < tmin || tmax < 0.0) return -1;

    float t = tmin < 0.0 ? tmax : tmin;
    return t;
}

float Intersections::intersectCylinder(const Ray& ray, vec3 pos, vec3 scale, vec3 rotation){
    mat3 rot = rotationMatrix(rotation);

    float ra = scale.x;
    float rb = scale.z;
    float height = scale.y;
    vec3 pa = pos + vec3(0.0f, height * 0.5f, 0.0f);
    vec3 pb = pos - vec3(0.0f, height * 0.5f, 0.0f);

    vec3 ba = pb - pa;
    vec3 oa = ray.origin - pa;
    vec3 ob = ray.origin - pb;
    float m0 = dot(ba,ba);
    float m1 = dot(oa,ba);
    float m2 = dot(ray.direction,ba);
    float m3 = dot(ray.direction,oa);
    float m5 = dot(oa,oa);
    float m9 = dot(ob,ba);

    if (m1 < 0.0f)
    {
        if (dot2(oa * m2 - ray.direction * m1) < (ra * ra * m2 * m2)){
            return -m1 / m2;
        }
    }
    else if (m9 > 0.0f)
    {
        float t = -m9 / m2;
        if (dot2(ob + ray.direction * t) < (rb * rb)){
            return t;
        }
    }
    
    // body
    float rr = ra - rb;
    float hy = m0 + rr*rr;
    float k2 = m0*m0 - m2*m2*hy;
    float k1 = m0*m0*m3 - m1*m2*hy + m0*ra*(rr*m2*1.0f);
    float k0 = m0*m0*m5 - m1*m1*hy + m0*ra*(rr*m1*2.0f - m0*ra);
    float h = k1*k1 - k2*k0;
    if (h < 0.0f) return -1.0f;
    float t = (-k1 - sqrt(h)) / k2;
    float y = m1 + t*m2;
    if (y < 0.0 || y > m0) return -1;

    return t;
}

float Intersections::intersectAABB(const Ray& invRay, const AABB& aabb, float tMin, float tMax) {
    vec3 t0 = (vec3(aabb.min) - invRay.origin) * invRay.direction;
    vec3 t1 = (vec3(aabb.max) - invRay.origin) * invRay.direction;

    vec3 tNear = min(t0, t1);
    vec3 tFar  = max(t0, t1);

    float tmin = glm::max(glm::max(tNear.x, tNear.y), glm::max(tNear.z, tMin));
    float tmax = glm::min(glm::min(tFar.x,  tFar.y),  glm::min(tFar.z,  tMax));

    if (tmax < tmin) return -1;
    return tmin;
}

float Intersections::intersectTriangle(const Ray& ray, const Triangle& triangle) {
    vec3 edge1 = triangle.v2 - triangle.v1;
    vec3 edge2 = triangle.v3 - triangle.v1;

    vec3 pvec = cross(ray.direction, edge2);
    float det = dot(edge1, pvec);

    if (abs(det) < 1e-5f)
        return -1;

    float invDet = 1.0f / det;
    vec3 tvec = ray.origin - triangle.v1;
    float u = dot(tvec, pvec) * invDet;
    if (u < 0.0f || u > 1.0f)
        return -1;

    vec3 qvec = cross(tvec, edge1);
    float v = dot(ray.direction, qvec) * invDet;
    if (v < 0.0f || u + v > 1.0f)
        return -1;

    float t = dot(edge2, qvec) * invDet;
    if (t < 0.001f) return -1;
    return t;
}

float Intersections::intersectMesh(const Ray& ray, shared_ptr<Mesh> mesh, vec3 pos, vec3 scale, vec3 rotation) {
    mat3 rot = rotationMatrix(rotation);
    mat3 invRot = transpose(rot);

    const vector<linBVHNode>& nodes = mesh->getLinNodes();
    const vector<Triangle>& triangles = mesh->getTriangles();

    float hitT = 1e6f;
    Ray newRay = ray;
    newRay.origin = invRot * (ray.origin - pos);
    newRay.direction = invRot * ray.direction;
    Ray invRay = newRay;
    invRay.direction = 1.0f / invRay.direction;

    float initialT = intersectAABB(invRay, Mesh::scaleAABB(nodes[(int)nodes.size() - 1].bounds, scale), 0.001f, hitT);
    if (initialT < 0) return -1.0f;

    const int STACK_SIZE = 32;

    int stack[STACK_SIZE];
    int stackPtr = 0;
    stack[stackPtr++] = (int)nodes.size() - 1;

    while (stackPtr > 0) {
        int nodeIndex = stack[--stackPtr];
        const linBVHNode& node = nodes[nodeIndex];

        float boxT = intersectAABB(invRay, Mesh::scaleAABB(node.bounds, scale), 0.001f, hitT);
        if (boxT < 0 || boxT > hitT) continue;

        if (node.leaf >= 0) {
            float triT = intersectTriangle(newRay, Mesh::scaleTri(triangles[node.leaf], scale));
            if (triT >= 0 && triT < hitT) {
                hitT = triT;
            }
        }
        else {
            float leftT = -1.0f;
            float rightT = -1.0f;
            if (node.left >= 0)
                leftT = intersectAABB(invRay, Mesh::scaleAABB(node.leftBounds, scale), 0.001f, hitT);
            if (node.right >= 0)
                rightT = intersectAABB(invRay, Mesh::scaleAABB(node.rightBounds, scale), 0.001f, hitT);

            if (leftT >= 0 && rightT >= 0){
                if (leftT < rightT) {
                    stack[stackPtr++] = node.right;
                    stack[stackPtr++] = node.left;
                } else {
                    stack[stackPtr++] = node.left;
                    stack[stackPtr++] = node.right;
                }
            } else if (leftT >= 0) stack[stackPtr++] = node.left;
            else if (rightT >= 0) stack[stackPtr++] = node.right;
        }
    }

    return hitT;
}
