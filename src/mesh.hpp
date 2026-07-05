#ifndef MESH
#define MESH

#include <glm/glm.hpp>
#include <vector>
#include <algorithm>
#include <string>
#include <filesystem>
#include <deque>

using namespace std;
using namespace glm;
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

struct AABB {
    vec4 min;
    vec4 max;

    void expand(const vec4& p) {
        min.x = std::min(min.x, p.x);
        min.y = std::min(min.y, p.y);
        min.z = std::min(min.z, p.z);
        max.x = std::max(max.x, p.x); 
        max.y = std::max(max.y, p.y);
        max.z = std::max(max.z, p.z);
    }

    void expand(const vec3& p) {
        min.x = std::min(min.x, p.x);
        min.y = std::min(min.y, p.y);
        min.z = std::min(min.z, p.z);
        max.x = std::max(max.x, p.x); 
        max.y = std::max(max.y, p.y);
        max.z = std::max(max.z, p.z);
    }

    void expand(const AABB& box) {
        expand(box.min);
        expand(box.max);
    }

    float volume(){
        vec4 e = max - min;
        return e.x * e.y * e.z;
    }

    float surfaceArea() {
        vec4 e = max - min;
        return 2.0f * (e.x*e.y + e.y*e.z + e.z*e.x);
    }
};

struct BVHNode {
    AABB bounds;
    AABB leftBounds;
    AABB rightBounds;
    shared_ptr<BVHNode> left = nullptr;
    shared_ptr<BVHNode> right = nullptr;
    int triangle = -1;
};

struct linBVHNode {
    int left = -1;
    int right = -1;
    int triangle = -1;
    int pad = -1;
    AABB bounds;
    AABB leftBounds;
    AABB rightBounds;
};

class Mesh {
public:
    Mesh();
    static AABB scaleAABB(AABB box, float scale);
    static Triangle scaleTri(Triangle tri, float scale);

    string modelPath = "";
    string modelName = "";

    void loadFromModel(const char* path);
    const vector<Triangle>& getTriangles() const;
    shared_ptr<BVHNode> computeBVH(vector<Triangle>& triangles, vector<int>& indices, int begin, int end);
    shared_ptr<BVHNode> computeSAH(vector<Triangle>& triangles, vector<int>& indices, int begin, int end);
    static vector<linBVHNode> lineariseBVH(shared_ptr<BVHNode> node);
    shared_ptr<BVHNode> getBVHNodes() const { return m_nodes; }
    const vector<linBVHNode>& getLinNodes() const { return m_linNodes; }
    
private:
    vector<Triangle> m_triangles = {};
    shared_ptr<BVHNode> m_nodes = nullptr;
    vector<linBVHNode> m_linNodes = {};
    static AABB triangleBounds(const Triangle& tri);
    static AABB computeBounds(const vector<Triangle>& triangles, vector<int>& indices, int begin, int end);
};

#endif