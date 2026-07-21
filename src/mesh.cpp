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
    m_nodes = BVH::computeSAH(computeBVHLeaves(m_triangles), triIndices, 0, size);
    m_linNodes = BVH::lineariseBVH(m_nodes);
    modelName = fs::path(path).stem().string();
    modelPath = path;
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

vector<BVHLeaf> Mesh::computeBVHLeaves(vector<Triangle>& triangles){
    vector<BVHLeaf> bvhTriangles;
    bvhTriangles.reserve(triangles.size());
    for(const Triangle& tri : triangles){
        bvhTriangles.push_back({
            tri.centroid(),
            triangleBounds(tri)
        });
    }
    return bvhTriangles;
}