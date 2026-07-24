#include "fur.hpp"

void Fur::loadFromBin(string binPath){
    std::ifstream f(binPath, std::ios::binary);
    if (!f.is_open()){
        std::cerr << "Cannot open : " << binPath << std::endl;
        return;
    }

    uint32_t pointCount;
    f.read((char*)&pointCount, sizeof(pointCount));
    if (!f){
        std::cerr << "Error reading point count from : " << binPath << std::endl;
        return;
    }
    m_points.resize(pointCount);
    f.read((char*)m_points.data(), pointCount * sizeof(HairPoint));
    if (!f){
        std::cerr << "Error reading points data from : " << binPath << std::endl;
        return;
    }

    uint32_t strandCount;
    f.read((char*)&strandCount, sizeof(strandCount));
    if (!f){
        std::cerr << "Error reading strand count from : " << binPath << std::endl;
        return;
    }
    m_strands.resize(strandCount);
    f.read((char*)m_strands.data(), strandCount * sizeof(ivec2));
    if (!f){
        std::cerr << "Error reading strands data from : " << binPath << std::endl;
        return;
    } 

    vector<BVHLeaf> leaves = computeBVHLeaves();
    int size = (int)leaves.size();
    vector<int> indicies(size);
    for (int i = 0; i < size; i++) { 
        indicies[i] = i;
    }

    shared_ptr<BVHNode> nodes = BVH::computeSAH(leaves, indicies, 0, size);
    m_linNodes = BVH::lineariseBVH(nodes);
    name = fs::path(binPath).stem().string();
    path = binPath;
}

void Fur::setData(vector<HairPoint> points, vector<ivec2> strands){
    m_points = points;
    m_strands = strands;
    
    vector<BVHLeaf> leaves = computeBVHLeaves();
    int size = (int)leaves.size();
    vector<int> indicies(size);
    for (int i = 0; i < size; i++) {
        indicies[i] = i;
    }

    shared_ptr<BVHNode> nodes = BVH::computeSAH(leaves, indicies, 0, size);
    m_linNodes = BVH::lineariseBVH(nodes);
}

vector<BVHLeaf> Fur::computeBVHLeaves(){
    vector<BVHLeaf> bvhLeaves;
    bvhLeaves.reserve(m_points.size() - m_strands.size());
    for(const ivec2& strand : m_strands){
        vec3 lastCentroid = vec3(0);
        for (int i = 0; i < strand.y - 1; i++){
            HairSegment seg = { m_points[strand.x + i], m_points[strand.x + i + 1] };
            lastCentroid = seg.centroid();
            bvhLeaves.push_back({
                lastCentroid,
                segmentBounds(seg)
            });
        }
        bvhLeaves.push_back({
            lastCentroid,
            {vec4(lastCentroid,1), vec4(lastCentroid, 1)}
        });
    }
    return bvhLeaves;
}

AABB Fur::segmentBounds(const HairSegment& seg){
    vec3 min1 = seg.start.p - vec3(seg.start.r);
    vec3 max1 = seg.start.p + vec3(seg.start.r);
    vec3 min2 = seg.end.p - vec3(seg.end.r);
    vec3 max2 = seg.end.p + vec3(seg.end.r);
    
    vec3 min = glm::min(min1, min2);
    vec3 max = glm::max(max1, max2);

    return {vec4(min, 1), vec4(max, 1)};
}
