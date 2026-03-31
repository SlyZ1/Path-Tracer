Hit sphereIntersect(Sphere sphere, Ray ray){
    vec3 oc = ray.origin - sphere.pos;
    float b = dot(oc, ray.dir);
    float c = dot(oc, oc) - sphere.rad * sphere.rad;
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

Hit lightIntersect(Light light, Ray ray){
    vec3 oc = ray.origin - light.pos;
    float b = dot(oc, ray.dir);
    float c = dot(oc, oc) - light.rad * light.rad;
    float h = b*b - c;
    Hit hit;
    hit.t = -1;
    if (h < 0.) return hit;
    float t = -b - sqrt(h);
    if (t <= 0) return hit;
    vec3 pos = ray.origin + t * ray.dir;
    Mat mat = Mat(MAT_EMIT, light.color, mData0(light.intensity));
    return Hit(t, normalize(pos - light.pos), mat, false);
}

Hit planeIntersect(Plane plane, Ray ray){
    vec3 rp = plane.origin - ray.origin;
    float t = dot(rp, plane.normal) / dot(ray.dir, plane.normal);
    vec3 relativePoint = ray.origin + t * ray.dir;
    vec3 difference = relativePoint - plane.origin;
    Hit hit;
    hit.t = -1;
    if (t <= 0 || dot(difference, difference) > 10000) 
        return hit;

    /*if (int((abs(relativePoint.x + 1) * 0.5) + int(abs(relativePoint.z + 1) * 0.5 + 1)) % 2 == 0) 
        plane.mat = Mat(plane.mat.type, vec3(0.83), mNoData());*/

    return Hit(t, plane.normal, plane.mat, false);
}

vec3 computeNormal(Triangle tri) {
    vec3 edge1 = tri.v1 - tri.v0;
    vec3 edge2 = tri.v2 - tri.v0;
    return normalize(cross(edge1, edge2));
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

    vec3 normal = computeNormal(tri);
    bool isInside = dot(normal, ray.dir) > 0;

    return Hit(t, normal, Mat(MAT_GLOSSY, metalProperties.xyz, mData(ballRoughness,metalProperties.w)), isInside);
}

Hit intersectAABB(Ray ray, AABB box, float tMin, float tMax)
{
    Hit hit;
    hit.t = -1.0f;
    
    float tmin = tMin;
    float tmax = tMax;
    
    for (int i = 0; i < 3; i++) {
        if (abs(ray.dir[i]) < 1e-8f) {
            continue;
        }
        
        float invD = 1 / ray.dir[i];
        float t0 = (box.min[i] - ray.origin[i]) * invD;
        float t1 = (box.max[i] - ray.origin[i]) * invD;
        
        if (invD < 0.0f) {
            float temp = t1;
            t1 = t0;
            t0 = temp;
        }
        
        tmin = max(tmin, t0);
        tmax = min(tmax, t1);
        
        if (tmax < tmin) {
            return hit;
        }
    }
    
    hit.t = tmin;
    hit.normal = -ray.dir;
    return hit;
}

Hit bvhIntersect(inout Ray ray)
{
    //ray.origin -= modelPos;
    const int STACK_SIZE = 32;

    int stack[STACK_SIZE];
    int stackPtr = 0;

    stack[stackPtr++] = numBVHNodes - 1;

    float hitT = 100000;
    float hitTri = -1;
    Hit hit;
    hit.t = -2;

    while (stackPtr > 0) {

        int nodeIndex = stack[--stackPtr];
        BVHNode node = nodes[nodeIndex];

        Hit boxHit = intersectAABB(ray, node.aabb, 0.001, hitT);
        if (boxHit.t < 0) continue;
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
            if (node.left >= 0)
                stack[stackPtr++] = node.left;

            if (node.right >= 0)
                stack[stackPtr++] = node.right;
        }
    }

    //ray.origin += modelPos;
    return hit;
}

Hit rayIntersection(World world, inout Ray ray){
    Hit hit;
    hit.t = 100000;
    for(int i = 0; i < NUM_SPHERE; i += 1){
        if (dot(world.spheres[i].pos - ray.origin, ray.dir) < 0 && false) continue;
        Hit sphereHit = sphereIntersect(world.spheres[i], ray);
        if (sphereHit.t > 0 && sphereHit.t < hit.t) hit = sphereHit;
    }
    for(int i = 0; i < NUM_PLANE; i += 1){
        if (dot(world.planes[i].normal, ray.dir) >= 0) continue;
        Hit planeHit = planeIntersect(world.planes[i], ray);
        if (planeHit.t > 0 && planeHit.t < hit.t) hit = planeHit;
    }
#if NUM_LIGHT > 0
    for(int i = 0; i < NUM_LIGHT; i += 1){
        Hit lightHit = lightIntersect(world.lights[i], ray);
        if (lightHit.t > 0 && lightHit.t < hit.t) hit = lightHit;
    }
#endif
    if (useModel){
        Hit bvhHit = bvhIntersect(ray);
        if (bvhHit.t > 0 && bvhHit.t < hit.t) hit = bvhHit;
    }

    if(hit.t == 100000) hit.t = -1;

    return hit;
}