#define PRIM_SPHERE 0
#define PRIM_PLANE 1
#define PRIM_CUBE 2

Hit sphereIntersect(Primitive sphere, Ray ray){
    vec3 oc = ray.origin - sphere.pos;
    float b = dot(oc, ray.dir);
    float c = dot(oc, oc) - sphere.scale * sphere.scale;
    float h = b*b - c;

    Hit hit;
    hit.t = -1;
    if (h < 0.) return hit;

    bool inside = false;
    float sqrtH = sqrt(h);
    float t = -b - sqrtH;
    if (t <= 0){
        t = -b + sqrtH;
        inside = true;
        if (t <= 0) return hit;
    }

    vec3 pos = ray.origin + t * ray.dir;
    vec3 normal = normalize(pos - sphere.pos);
    return Hit(t, normal, sphere.mat, inside);
}

Hit planeIntersect(Primitive plane, Ray ray){
    vec3 rp = plane.pos - ray.origin;
    vec3 normal = vec3(0,1,0);
    float t = dot(rp, normal) / dot(ray.dir, normal);
    vec3 relativePoint = ray.origin + t * ray.dir;
    vec3 difference = relativePoint - plane.pos;
    Hit hit;
    hit.t = -1;
    if (t <= 0 || dot(difference, difference) > 10000) 
        return hit;

    /*if (int((abs(relativePoint.x + 1) * 0.5) + int(abs(relativePoint.z + 1) * 0.5 + 1)) % 2 == 0) 
        plane.mat = Mat(plane.mat.type, vec3(0.83), mNoData());*/

    return Hit(t, normal, plane.mat, false);
}

Hit triangleIntersect(Triangle tri, Ray ray){
    Hit hit; hit.t = -2;

    vec3 edge1 = tri.v1 - tri.v0;
    vec3 edge2 = tri.v2 - tri.v0;

    vec3 pvec = cross(ray.dir, edge2);
    float det = dot(edge1, pvec);

    if (abs(det) < PROBA_EPS)
        return hit; // rayon parallèle

    float invDet = 1.0 / det;
    vec3 tvec = ray.origin - tri.v0;
    float u = dot(tvec, pvec) * invDet;
    if (u < 0.0 || u > 1.0)
        return hit;

    vec3 qvec = cross(tvec, edge1);
    float v = dot(ray.dir, qvec) * invDet;
    if (v < 0.0 || u + v > 1.0)
        return hit;

    float t = dot(edge2, qvec) * invDet;
    if (t < 0.001) return hit;

    vec3 normal = normalize(tri.n0 * (1-u-v) + tri.n1 * u + tri.n2 * v);
    bool isInside = dot(normal, ray.dir) > 0;

    return Hit(t, normal, Mat(vec3(1), 2, mData(0,1.3), vec2(0)), isInside);
}

Hit intersectAABB(Ray invRay, AABB box, float tMin, float tMax)
{
    Hit hit;
    hit.t = -1.0f;
    
    vec3 t0 = (box.min - invRay.origin) * invRay.dir;
    vec3 t1 = (box.max - invRay.origin) * invRay.dir;

    vec3 tNear = min(t0, t1);
    vec3 tFar  = max(t0, t1);

    float tmin = max(max(tNear.x, tNear.y), max(tNear.z, tMin));
    float tmax = min(min(tFar.x,  tFar.y),  min(tFar.z,  tMax));

    if (tmax < tmin) return hit;

    hit.t = tmin;
    return hit;
}

Hit bvhIntersect(inout Ray ray)
{
    //ray.origin -= modelPos;
    const int STACK_SIZE = 32;

    int stack[STACK_SIZE];
    int stackPtr = 0;

    stack[stackPtr++] = numBVHNodes - 1;

    float hitT = 1e6;
    float hitTri = -1;
    Hit hit;
    hit.t = -2;
    Ray invRay = ray;

    while (stackPtr > 0) {
        int nodeIndex = stack[--stackPtr];
        BVHNode node = nodes[nodeIndex];

        Hit boxHit = intersectAABB(invRay, node.aabb, 0.001, hitT);
        if (boxHit.t < 0 || boxHit.t > hitT) continue;
        if (debugBVH > 0) ray.throughput *= 0.95;

        if (node.triangle >= 0) {
            Hit triHit = triangleIntersect(triangles[node.triangle], ray);
            if (triHit.t >= 0) {
                if (triHit.t < hitT) {
                    hitT = triHit.t;
                    hitTri = node.triangle;
                    hit = triHit;
                }
            }
        }
        else {
            Hit leftHit; leftHit.t = -1;
            Hit rightHit; rightHit.t = -1;
            if (node.left >= 0)
                leftHit = intersectAABB(invRay, nodes[node.left].aabb, 0.001, hitT);
            if (node.right >= 0)
                rightHit = intersectAABB(invRay, nodes[node.right].aabb, 0.001, hitT);

            if (leftHit.t >= 0 && rightHit.t >= 0){
                if (leftHit.t < rightHit.t) {
                    stack[stackPtr++] = node.right;
                    stack[stackPtr++] = node.left;
                } else {
                    stack[stackPtr++] = node.left;
                    stack[stackPtr++] = node.right;
                }
            } else if (leftHit.t >= 0) stack[stackPtr++] = node.left;
            else if (rightHit.t >= 0) stack[stackPtr++] = node.right;
        }
    }

    //ray.origin += modelPos;
    return hit;
}

Hit rayIntersection(inout Ray ray){
    Hit hit;
    hit.t = 100000;
    for(int i = 0; i < numPrimitives; i += 1){
        Primitive prim = primitives[i];
        Hit newHit;
        switch (prim.type){
            case PRIM_SPHERE:
                newHit = sphereIntersect(prim, ray);
                break;
            case PRIM_PLANE:
                newHit = planeIntersect(prim, ray);
                break; 
        }
        if (newHit.t > 0 && newHit.t < hit.t) hit = newHit;
    }
    if (useModel){
        Hit bvhHit = bvhIntersect(ray);
        if (bvhHit.t > 0 && bvhHit.t < hit.t) hit = bvhHit;
    }

    if(hit.t == 100000) hit.t = -1;

    return hit;
}