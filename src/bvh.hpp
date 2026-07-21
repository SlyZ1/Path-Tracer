#ifndef BVH_HPP
#define BVH_HPP

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <algorithm>

using namespace std;
using namespace glm;

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

struct BVHLeaf {
    vec3 centroid;
    AABB bounds;
};

struct BVHNode {
    AABB bounds;
    AABB leftBounds;
    AABB rightBounds;
    shared_ptr<BVHNode> left = nullptr;
    shared_ptr<BVHNode> right = nullptr;
    int leaf = -1;
};

struct linBVHNode {
    int left = -1;
    int right = -1;
    int leaf = -1;
    int pad = -1;
    AABB bounds;
    AABB leftBounds;
    AABB rightBounds;
};

class BVH {
public:
    static AABB computeBounds(const vector<BVHLeaf>& leaves, vector<int>& indices, int begin, int end);
    static shared_ptr<BVHNode> computeBVH(vector<BVHLeaf>& leaves, vector<int>& indices, int begin, int end);
    static shared_ptr<BVHNode> computeSAH(vector<BVHLeaf>& leaves, vector<int>& indices, int begin, int end);
    static vector<linBVHNode> lineariseBVH(shared_ptr<BVHNode> node);
private:
    static int lineariseRec(shared_ptr<BVHNode> node, vector<linBVHNode>& nodes);
};

#endif