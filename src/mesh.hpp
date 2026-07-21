#ifndef MESH
#define MESH

#include <string>
#include <filesystem>
#include <deque>
#include "bvh.hpp"

namespace fs = std::filesystem;

struct Triangle {
    vec3 v1;
    float pad1;
    vec3 v2;
    float pad2;
    vec3 v3;
    float pad3;
    vec3 n1;
    float pad4;
    vec3 n2;
    float pad5;
    vec3 n3;
    float pad6;

    vec3 centroid() const {
        return {(v1.x + v3.x + v2.x) / 3.0f,
                (v1.y + v3.y + v2.y) / 3.0f,
                (v1.z + v3.z + v2.z) / 3.0f};
    }
};

class Mesh {
public:
    Mesh();
    static AABB scaleAABB(AABB box, vec3 scale);
    static Triangle scaleTri(Triangle tri, vec3 scale);

    string modelPath = "";
    string modelName = "";

    void loadFromModel(const char* path);
    const vector<Triangle>& getTriangles() const { return m_triangles; }
    const vector<linBVHNode>& getLinNodes() const { return m_linNodes; }
    
private:
    vector<Triangle> m_triangles = {};
    vector<linBVHNode> m_linNodes = {};
    vector<BVHLeaf> computeBVHLeaves();
    static AABB triangleBounds(const Triangle& tri);
};

#endif