#include "fur.hpp"

void Fur::loadFromBin(string path){
    {
        std::ifstream f(path + "_points.bin", std::ios::binary);
        if (!f.is_open()){
            std::cerr << "Cannot open : " << (path + "_points.bin") << std::endl;
            return;
        }
        uint32_t count;
        f.read((char*)&count, sizeof(count));
        m_points.resize(count);
        f.read((char*)m_points.data(), count * sizeof(HairPoint));
    }
    {
        std::ifstream f(path + "_strands.bin", std::ios::binary);
        if (!f.is_open()){
            std::cerr << "Cannot open : " << (path + "_strands.bin") << std::endl;
            return;
        }
        uint32_t count;
        f.read((char*)&count, sizeof(count));
        m_strands.resize(count);
        f.read((char*)m_strands.data(), count * sizeof(ivec2));
    }

    vector<BVHLeaf> leaves = computeBVHLeaves();
    int size = (int)leaves.size();
    vector<int> indicies(size);
    for (int i = 0; i < size; i++) {
        indicies[i] = i;
    }

    shared_ptr<BVHNode> nodes = BVH::computeSAH(leaves, indicies, 0, size);
    m_linNodes = BVH::lineariseBVH(nodes);
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
    for (linBVHNode node : m_linNodes){
        cout << Utils::toString(node.bounds.min) << " " << Utils::toString(node.bounds.max) << endl;
    }
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
