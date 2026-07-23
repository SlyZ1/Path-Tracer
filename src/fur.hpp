#ifndef FUR_HPP
#define FUR_HPP

#include "bvh.hpp"
#include <iostream>
#include <fstream>
#include <cstdint>
#include "utils.hpp"
#include <filesystem>

namespace fs = std::filesystem;

struct HairPoint {
    vec3 p;
    float r;
};

struct HairSegment {
    HairPoint start;
    HairPoint end;

    vec3 centroid() const {
        return (start.p + end.p) * 0.5f;
    }
};

class Fur {
public:
    string furPath = "";
    string furName = "";

    void loadFromBin(string path);
    void setData(vector<HairPoint> points, vector<ivec2> strands);
    const vector<HairPoint> getPoints() const { return m_points; }
    const vector<ivec2>& getStrands() const { return m_strands; }
    const vector<linBVHNode>& getLinNodes() const { return m_linNodes; }

private:
    vector<HairPoint> m_points = {};
    vector<ivec2> m_strands = {};
    vector<linBVHNode> m_linNodes = {};
    vector<BVHLeaf> computeBVHLeaves();
    static AABB segmentBounds(const HairSegment& seg);
};

#endif