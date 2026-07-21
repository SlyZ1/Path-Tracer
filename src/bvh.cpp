#include "bvh.hpp"


AABB BVH::computeBounds(const vector<BVHLeaf>& leaves, vector<int>& indices, int begin, int end){
    AABB box = leaves[indices[begin]].bounds;
    for(int i = begin + 1; i < end; ++i)
        box.expand(leaves[indices[i]].bounds);
    return box;
}

shared_ptr<BVHNode> BVH::computeBVH(vector<BVHLeaf>& leaves, vector<int>& indices, int begin, int end){
    shared_ptr<BVHNode> node = make_shared<BVHNode>();
    node->bounds = computeBounds(leaves, indices, begin, end);

    const int MAX_OBJECT_PER_LEAF = 1;
    if (end - begin <= MAX_OBJECT_PER_LEAF) {
        node->leaf = indices[begin]; 
        return node;
    }

    AABB centroidAABB;
    centroidAABB.min = centroidAABB.max = vec4(leaves[indices[begin]].centroid, 0);
    for (int i = begin + 1; i < end; ++i)
        centroidAABB.expand(leaves[indices[i]].centroid);

    vec4 extent = centroidAABB.max - centroidAABB.min;
    int axis = 0;
    if (extent.y > extent.x && extent.y > extent.z)
        axis = 1;
    else if (extent.z > extent.x)
        axis = 2;

    std::sort(indices.begin() + begin, indices.begin() + end,
        [&leaves, axis](int a, int b) {
            return leaves[a].centroid[axis] < leaves[b].centroid[axis];
        });

    int mid = begin + (end - begin) / 2;

    node->left = computeBVH(leaves, indices, begin, mid);
    node->right = computeBVH(leaves, indices, mid, end);

    return node;
}

shared_ptr<BVHNode> BVH::computeSAH(vector<BVHLeaf>& leaves, vector<int>& indices, int begin, int end){
    shared_ptr<BVHNode> node = make_shared<BVHNode>();
    node->bounds = computeBounds(leaves, indices, begin, end);
    node->leaf = -1;
    
    const int MAX_OBJECT_PER_LEAF = 1;
    if (end - begin <= MAX_OBJECT_PER_LEAF) {
        node->leaf = indices[begin];
        node->leftBounds = {vec4(0), vec4(0)};
        node->rightBounds = {vec4(0), vec4(0)};
        return node;
    }
    
    AABB centroidAABB;
    centroidAABB.min = centroidAABB.max = vec4(leaves[indices[begin]].centroid, 0);
    for (int i = begin + 1; i < end; i++){
        centroidAABB.expand(leaves[indices[i]].centroid);
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
        const BVHLeaf& lea = leaves[indices[i]];
        float t = (lea.centroid[axis] - centroidAABB.min[axis]) / extent[axis];
        int b = std::clamp((int)(t * NUM_BUCKETS), 0, NUM_BUCKETS - 1);
        buckets[b].bounds.expand(lea.bounds);
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
            return leaves[idx].centroid[axis] <= bestSplitPoint;
        });

    int splitMid = (int)(mid - indices.begin());
    splitMid = std::clamp(splitMid, begin + 1, end - 1);

    node->left = computeSAH(leaves, indices, begin, splitMid);
    node->right = computeSAH(leaves, indices, splitMid, end);
    node->leftBounds = node->left->bounds;
    node->rightBounds = node->right->bounds;

    return node;
}

int BVH::lineariseRec(shared_ptr<BVHNode> node, vector<linBVHNode>& nodes){
    if (node == nullptr){
        return -1;
    }

    linBVHNode linNode;
    linNode.left = lineariseRec(node->left, nodes);
    linNode.right = lineariseRec(node->right, nodes);
    linNode.leaf = node->leaf;
    linNode.pad = 0;
    linNode.bounds = node->bounds;
    linNode.leftBounds = node->leftBounds;
    linNode.rightBounds = node->rightBounds;

    int i = (int)nodes.size();
    nodes.push_back(linNode);
    return i;
}

vector<linBVHNode> BVH::lineariseBVH(shared_ptr<BVHNode> node){
    vector<linBVHNode> nodes = {};
    lineariseRec(node, nodes);
    return nodes;
}
