#ifndef INTERSECTION_HPP
#define INTERSECTION_HPP

#include <glm/glm.hpp>
#include "mesh.hpp"

using namespace glm;

enum PrimType : int {
    SPHERE = 0,
    PLANE = 1,
    CUBE = 2,
    CYLINDER = 3,
    MESH_ = 4,
    FUR_ = 5
};

struct Ray {
    vec3 origin;
    vec3 direction; 
};

class Intersections {
private:
    static float dot2(vec3 x);
public:
    static mat3 rotationMatrix(vec3 rotationDegrees);
    static float intersectSphere(const Ray& ray, vec3 pos, vec3 scale, vec3 rotation);
    static float intersectPlane(const Ray& ray, vec3 pos, vec3 scale, vec3 rotation);
    static float intersectCube(const Ray& ray, vec3 pos, vec3 scale, vec3 rotation);
    static float intersectCylinder(const Ray& ray, vec3 pos, vec3 scale, vec3 rotation);
    static float intersectAABB(const Ray& invRay, const AABB& aabb, float tMin, float tMax);
    static float intersectTriangle(const Ray& ray, const Triangle& triangle);
    static float intersectMesh(const Ray& ray, shared_ptr<Mesh> mesh, vec3 pos, vec3 scale, vec3 rotation);
};

#endif