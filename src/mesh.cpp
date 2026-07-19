#include "mesh.hpp"
#include <tinyobjloader/tiny_obj_loader.h>
#include <iostream>
#include <fstream>
#include <stdio.h>

using namespace std;

Mesh::Mesh() {}

AABB Mesh::scaleAABB(AABB box, vec3 scale){
    box.min *= vec4(scale, 1.0);
    box.max *= vec4(scale, 1.0);
    return box;
}

Triangle Mesh::scaleTri(Triangle tri, vec3 scale){
    tri.v1 *= scale;
    tri.v2 *= scale;
    tri.v3 *= scale;
    return tri;
}

void Mesh::loadFromModel(const char* path){
    if (fs::path(path).extension().string() != ".obj") return;

    cout << "Loading 3D model and computing BVH... (at " << path << ")" << endl;
    ifstream ifs(path);
    if (!ifs) {
        cerr << "Cannot open OBJ file\n" << endl;
        return;
    }

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    tinyobj::LoadObj(
        &attrib,
        &shapes,
        &materials,
        &warn,
        &err,
        &ifs,
        nullptr
    );

    if (!warn.empty()) cout << warn << endl;
    if (!err.empty()){
        cerr << err << endl;
        return;
    }
    
    auto getPos = [&](tinyobj::index_t i){
        return vec3(
            attrib.vertices[3 * i.vertex_index + 0],
            attrib.vertices[3 * i.vertex_index + 1],
            attrib.vertices[3 * i.vertex_index + 2]
        );
    };

    auto getNormal = [&](tinyobj::index_t i){
        if (i.normal_index >= 0) {
            return vec3(
                attrib.normals[3 * i.normal_index + 0],
                attrib.normals[3 * i.normal_index + 1],
                attrib.normals[3 * i.normal_index + 2]
            );
        }
        return vec3(0.0f); // pas de normale dans le fichier
    };

    for(const auto &shape : shapes){
        const auto &mesh = shape.mesh;
        int numVertex = 3;
        int size = (int)mesh.num_face_vertices.size();
        int index_offset = 0;
        for (int i = 0; i < size; i++)
        {
            int fv = mesh.num_face_vertices[i];
            if (fv == 3){
                auto i0 = mesh.indices[numVertex * i + 0];
                auto i1 = mesh.indices[numVertex * i + 1];
                auto i2 = mesh.indices[numVertex * i + 2];
    
                Triangle newTriangle = Triangle();
                newTriangle.v1 = getPos(i0);
                newTriangle.v2 = getPos(i1);
                newTriangle.v3 = getPos(i2);
                newTriangle.n1 = getNormal(i0);
                newTriangle.n2 = getNormal(i1);
                newTriangle.n3 = getNormal(i2);
                
                m_triangles.push_back(newTriangle);   
            }
            index_offset += fv;
        }
    }

    int size = (int)m_triangles.size();
    vector<int> triIndices(size);
    for (int i = 0; i < size; i++) {
        triIndices[i] = i;
    }
    m_nodes = computeSAH(computeBVHTriangles(m_triangles), triIndices, 0, size);
    m_linNodes = lineariseBVH(m_nodes);
    modelName = fs::path(path).stem().string();
    modelPath = path;
}

const vector<Triangle>& Mesh::getTriangles() const {
    return m_triangles;
}

AABB Mesh::triangleBounds(const Triangle& tri) {
    AABB box;
    box.min =  {std::min({tri.v3.x, tri.v1.x, tri.v2.x}),
                std::min({tri.v3.y, tri.v1.y, tri.v2.y}),
                std::min({tri.v3.z, tri.v1.z, tri.v2.z}),
                0};
    box.max =  {std::max({tri.v3.x, tri.v1.x, tri.v2.x}),
                std::max({tri.v3.y, tri.v1.y, tri.v2.y}),
                std::max({tri.v3.z, tri.v1.z, tri.v2.z}),
                0};
    return box;
}

vector<Mesh::BVHTriangle> Mesh::computeBVHTriangles(vector<Triangle>& triangles){
    vector<Mesh::BVHTriangle> bvhTriangles;
    bvhTriangles.reserve(triangles.size());
    for(const Triangle& tri : triangles){
        bvhTriangles.push_back({
            tri.centroid(),
            triangleBounds(tri)
        });
    }
    return bvhTriangles;
}

AABB Mesh::computeBounds(const vector<BVHTriangle>& triangles, vector<int>& indices, int begin, int end) {
    AABB box = triangles[indices[begin]].bounds;
    for(int i = begin + 1; i < end; ++i)
        box.expand(triangles[indices[i]].bounds);
    return box;
}

shared_ptr<BVHNode> Mesh::computeBVH(vector<BVHTriangle>& triangles, 
                          vector<int>& indices,
                          int begin, int end) 
{
    shared_ptr<BVHNode> node = make_shared<BVHNode>();
    node->bounds = computeBounds(triangles, indices, begin, end);

    const int MAX_TRIANGLES_PER_LEAF = 1;
    if (end - begin <= MAX_TRIANGLES_PER_LEAF) {
        node->triangle = indices[begin]; 
        return node;
    }

    AABB centroidAABB;
    centroidAABB.min = centroidAABB.max = vec4(triangles[indices[begin]].centroid, 0);
    for (int i = begin + 1; i < end; ++i)
        centroidAABB.expand(triangles[indices[i]].centroid);

    vec4 extent = centroidAABB.max - centroidAABB.min;
    int axis = 0;
    if (extent.y > extent.x && extent.y > extent.z)
        axis = 1;
    else if (extent.z > extent.x)
        axis = 2;

    std::sort(indices.begin() + begin, indices.begin() + end,
        [&triangles, axis](int a, int b) {
            return triangles[a].centroid[axis] < triangles[b].centroid[axis];
        });

    int mid = begin + (end - begin) / 2;

    node->left = computeBVH(triangles, indices, begin, mid);
    node->right = computeBVH(triangles, indices, mid, end);

    return node;
}

shared_ptr<BVHNode> Mesh::computeSAH(vector<BVHTriangle>& triangles, vector<int>& indices, int begin, int end){
    shared_ptr<BVHNode> node = make_shared<BVHNode>();
    node->bounds = computeBounds(triangles, indices, begin, end);
    node->triangle = -1;
    
    const int MAX_TRIANGLES_PER_LEAF = 1;
    if (end - begin <= MAX_TRIANGLES_PER_LEAF) {
        node->triangle = indices[begin];
        node->leftBounds = {vec4(0), vec4(0)};
        node->rightBounds = {vec4(0), vec4(0)};
        return node;
    }
    
    AABB centroidAABB;
    centroidAABB.min = centroidAABB.max = vec4(triangles[indices[begin]].centroid, 0);
    for (int i = begin + 1; i < end; i++){
        centroidAABB.expand(triangles[indices[i]].centroid);
    }

    vec4 extent = centroidAABB.max - centroidAABB.min;
    int axis = 0;
    if (extent.y > extent.x && extent.y > extent.z)
        axis = 1;
    else if (extent.z > extent.x)
        axis = 2;

    const int NUM_BUCKETS = 16;
    struct Bucket { AABB bounds; int count = 0; };
    Bucket buckets[NUM_BUCKETS];

    for (int i = begin; i < end; i++){
        const BVHTriangle& tri = triangles[indices[i]];
        float t = (tri.centroid[axis] - centroidAABB.min[axis]) / extent[axis];
        int b = std::clamp((int)(t * NUM_BUCKETS), 0, NUM_BUCKETS - 1);
        buckets[b].bounds.expand(tri.bounds);
        buckets[b].count++;
    }

    AABB leftBounds[NUM_BUCKETS];
    int leftCounts[NUM_BUCKETS];
    AABB acc; int count = 0;
    for (int i = 0; i < NUM_BUCKETS; i++){
        if (buckets[i].count > 0){
            acc.expand(buckets[i].bounds);
            count += buckets[i].count;
        }
        leftBounds[i] = acc;
        leftCounts[i] = count;
    }

    AABB rightBounds[NUM_BUCKETS];
    int rightCounts[NUM_BUCKETS];
    acc = {}; count = 0;
    for (int i = NUM_BUCKETS - 1; i >= 0; i--){
        if (buckets[i].count > 0){
            acc.expand(buckets[i].bounds);
            count += buckets[i].count;
        }
        rightBounds[i] = acc;
        rightCounts[i] = count;
    }

    float minCost = FLT_MAX;
    float bestSplitPoint = 0;
    for (int i = 0; i < NUM_BUCKETS - 1; i++){
        float cost = leftCounts[i] * leftBounds[i].surfaceArea() 
                + rightCounts[i+1] * rightBounds[i+1].surfaceArea();
        if (cost < minCost){
            minCost = cost;
            bestSplitPoint = centroidAABB.min[axis] + (float)(i+1) / NUM_BUCKETS * extent[axis];
        }
    }

    auto mid = std::partition(indices.begin() + begin, indices.begin() + end,
        [&](int idx) {
            return triangles[idx].centroid[axis] <= bestSplitPoint;
        });

    int splitMid = (int)(mid - indices.begin());
    splitMid = std::clamp(splitMid, begin + 1, end - 1);

    node->left = computeSAH(triangles, indices, begin, splitMid);
    node->right = computeSAH(triangles, indices, splitMid, end);
    node->leftBounds = node->left->bounds;
    node->rightBounds = node->right->bounds;

    return node;
}

int indexOfTriangle(const vector<Triangle>& triangles, const Triangle& triangle){
    int size = (int)triangles.size();
    for(int i = 0; i < size; i++){
        Triangle tri = triangles[i];
        if (tri.v1 == triangle.v1 && tri.v2 == triangle.v2 && tri.v3 == triangle.v3){
            return i;
        }
    }
    return -1;
}

int lineariseRec(shared_ptr<BVHNode> node, vector<linBVHNode>& nodes){
    if (node == nullptr){
        return -1;
    }

    linBVHNode linNode;
    linNode.left = lineariseRec(node->left, nodes);
    linNode.right = lineariseRec(node->right, nodes);
    linNode.triangle = node->triangle;
    linNode.pad = 0;
    linNode.bounds = node->bounds;
    linNode.leftBounds = node->leftBounds;
    linNode.rightBounds = node->rightBounds;

    int i = (int)nodes.size();
    nodes.push_back(linNode);
    return i;
}

vector<linBVHNode> Mesh::lineariseBVH(shared_ptr<BVHNode> node){
    vector<linBVHNode> nodes = {};
    lineariseRec(node, nodes);
    return nodes;
}