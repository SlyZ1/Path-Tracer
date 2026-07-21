#define PRIM_SPHERE 0
#define PRIM_PLANE 1
#define PRIM_CUBE 2
#define PRIM_CYLINDER 3

Hit sphereIntersect(Primitive sphere, Ray ray){
    mat3 rot = rotationMatrix(sphere.rotation);
    mat3 invRot = transpose(rot);

    vec3 dir = (invRot * ray.dir) / sphere.scale;
    vec3 oc = (invRot * (ray.origin - sphere.pos)) / sphere.scale;

    float a = dot(dir, dir);
    float b = dot(oc, dir);
    float c = dot(oc, oc) - 1;

    float h = b*b - a*c;

    Hit hit;
    hit.t = -1;
    if (h < 0.) return hit;

    bool inside = false;
    float sqrtH = sqrt(h);
    float t = (-b - sqrtH) / a;
    if (t <= 0){
        t = (-b + sqrtH) / a;
        inside = true;
        if (t <= 0) return hit;
    }

    vec3 localHit = oc + t * dir;
    vec3 normal = normalize(rot * (localHit / sphere.scale));

    Hit newHit;
    newHit.t = t;
    newHit.normal = normal;
    newHit.inside = inside;
    return newHit;
}

Hit planeIntersect(Primitive plane, Ray ray){
    mat3 rot = rotationMatrix(plane.rotation);
    mat3 invRot = transpose(rot);

    vec3 localOrigin = invRot * (ray.origin - plane.pos);
    vec3 localDir = invRot * ray.dir;

    vec3 normal = vec3(0,1,0);
    float t = -localOrigin.y / localDir.y;
    vec3 localHit = localOrigin + t * localDir;
    vec3 difference = abs(localHit);
    Hit hit;
    hit.t = -1;
    vec3 scale = plane.scale / 2.0;
    if (t <= 0 || difference.x > scale.x || difference.z > scale.z)
        return hit;

    Hit newHit;
    newHit.t = t;
    newHit.normal = normalize(rot * normal);
    newHit.inside = false;
    return newHit;
}

Hit triangleIntersect(Triangle tri, Ray ray, bool isSmooth){
    Hit emptyHit; emptyHit.t = -2;
    
    vec3 edge1 = tri.v1 - tri.v0;
    vec3 edge2 = tri.v2 - tri.v0;

    vec3 pvec = cross(ray.dir, edge2);
    float det = dot(edge1, pvec);

    if (abs(det) < PROBA_EPS)
        return emptyHit;

    float invDet = 1.0 / det;
    vec3 tvec = ray.origin - tri.v0;
    float u = dot(tvec, pvec) * invDet;
    if (u < 0.0 || u > 1.0)
        return emptyHit;

    vec3 qvec = cross(tvec, edge1);
    float v = dot(ray.dir, qvec) * invDet;
    if (v < 0.0 || u + v > 1.0)
        return emptyHit;

    float t = dot(edge2, qvec) * invDet;
    if (t < 0.001) return emptyHit;

    vec3 geomNormal = cross(edge1, edge2);
    vec3 normal = vec3(0);
    if (isSmooth){
        normal = normalize(tri.n0 * (1-u-v) + tri.n1 * u + tri.n2 * v);
        if (dot(normal, geomNormal) < 0.0) normal = -normal;
    }
    else{
        normal = (tri.n0 + tri.n1 + tri.n2) / 3.0;
    }
    bool isInside = dot(geomNormal, ray.dir) > 0;
    Hit newHit;
    newHit.t = t;
    newHit.normal = normal;
    newHit.inside = isInside;
    return newHit;
}

Hit intersectAABB(Ray invRay, AABB box, float tMin, float tMax)
{
    Hit hit;
    hit.t = -1.0f;
    
    vec3 t0 = (box.min.xyz - invRay.origin) * invRay.dir;
    vec3 t1 = (box.max.xyz - invRay.origin) * invRay.dir;

    vec3 tNear = min(t0, t1);
    vec3 tFar  = max(t0, t1);

    float tmin = max(max(tNear.x, tNear.y), max(tNear.z, tMin));
    float tmax = min(min(tFar.x,  tFar.y),  min(tFar.z,  tMax));

    if (tmax < tmin) return hit;

    hit.t = tmin;
    return hit;
}

Hit intersectCube(Primitive cube, Ray ray)
{
    Hit hit;
    hit.t = -1.0;

    mat3 rot = rotationMatrix(cube.rotation);
    mat3 invRot = transpose(rot);

    vec3 localOrigin = invRot * (ray.origin - cube.pos);
    vec3 localDir = invRot * ray.dir;

    vec3 minP = -cube.scale;
    vec3 maxP = cube.scale;

    vec3 t0 = (minP - localOrigin) / localDir;
    vec3 t1 = (maxP - localOrigin) / localDir;

    vec3 tNear = min(t0, t1);
    vec3 tFar = max(t0, t1);

    float tmin = max(tNear.x, max(tNear.y, tNear.z));
    float tmax = min(tFar.x,  min(tFar.y,  tFar.z));

    if (tmax < tmin || tmax < 0.0) return hit;

    bool inside = tmin < 0.0;
    float t = inside ? tmax : tmin;

    vec3 normal;
    vec3 localHitPoint = localOrigin + localDir * t;
    vec3 d = localHitPoint / cube.scale;

    vec3 absD = abs(d);
    if (absD.x > absD.y && absD.x > absD.z)
        normal = vec3(sign(d.x), 0.0, 0.0);
    else if (absD.y > absD.z)
        normal = vec3(0.0, sign(d.y), 0.0);
    else
        normal = vec3(0.0, 0.0, sign(d.z));

    hit.t = t;
    hit.normal = normalize(rot * normal);
    hit.inside = inside;
    return hit;
}

Hit intersectCylinder(Primitive cylinder, Ray ray){
    Hit hit;
    hit.t = -1.0;

    mat3 rot = rotationMatrix(cylinder.rotation);

    float ra = cylinder.scale.x;
    float rb = cylinder.scale.z;
    float height = cylinder.scale.y;
    vec3 pa = cylinder.pos + vec3(0, height * 0.5, 0);
    vec3 pb = cylinder.pos - vec3(0, height * 0.5, 0);

    vec3 ba = pb - pa;
    vec3 oa = ray.origin - pa;
    vec3 ob = ray.origin - pb;
    float m0 = dot(ba,ba);
    float m1 = dot(oa,ba);
    float m2 = dot(ray.dir,ba);
    float m3 = dot(ray.dir,oa);
    float m5 = dot(oa,oa);
    float m9 = dot(ob,ba); 
    float rr = ra - rb;
    
    float y0 = m1;
    float radialDist2 = m5 - m1*m1/m0;
    float r0 = ra - rr * y0 / m0;
    bool inside = (radialDist2 < r0*r0) && (y0 > 0.0) && (y0 < m0);
    hit.inside = inside;

    // caps
    if (inside)
    {
        if (m2 > 0.0)
        {
            float t = (m0 - m1) / m2;
            vec3 p = oa + ray.dir * t;
            if (dot2(p - ba * (dot(p,ba) / m0)) < rb * rb)
            {
                hit.t = -m1 / m2;
                hit.normal = -ba * inversesqrt(m0);
                return hit;
            }
        }
        else if (m2 < 0.0)
        {
            float t = -m1 / m2;
            vec3 p = oa + ray.dir * t;
            if (dot2(p - ba * (dot(p,ba) / m0)) < ra * ra)
            {
                hit.t = -m1 / m2;
                hit.normal = -ba * inversesqrt(m0);
                return hit;
            }
        }
    }
    else{

        if (m1 < 0.0)
        {
            if (dot2(oa * m2 - ray.dir * m1) < (ra * ra * m2 * m2)){
                hit.t = -m1 / m2;
                hit.normal = -ba * inversesqrt(m0);
                return hit;
            }
        }
        else if (m9 > 0.0)
        {
            float t = -m9 / m2;
            if (dot2(ob + ray.dir * t) < (rb * rb)){
                hit.t = t;
                hit.normal = ba * inversesqrt(m0);
                return hit;
            }
        }
    }
    
    // body
    float hy = m0 + rr*rr;
    float k2 = m0*m0 - m2*m2*hy;
    float k1 = m0*m0*m3 - m1*m2*hy + m0*ra*(rr*m2*1.0);
    float k0 = m0*m0*m5 - m1*m1*hy + m0*ra*(rr*m1*2.0 - m0*ra);
    float h = k1*k1 - k2*k0;
    if (h < 0.0) return hit;
    float t = inside ? (-k1 + sqrt(h)) / k2 : (-k1 - sqrt(h)) / k2;
    float y = m1 + t*m2;
    if (y < 0.0 || y > m0) return hit;

    hit.t = t;
    hit.normal = normalize(m0*(m0*(oa+t*ray.dir)+rr*ba*ra)-ba*hy*y);
    return hit;
}

AABB scaleAABB(AABB box, vec3 scale){
    box.min.xyz *= scale;
    box.max.xyz *= scale;
    return box;
}

Triangle scaleTri(Triangle tri, vec3 scale){
    tri.v0 *= scale;
    tri.v1 *= scale;
    tri.v2 *= scale;
    return tri;
}

Hit bvhIntersect(inout Ray ray, MeshInfos info, bool isShadow)
{
    vec3 pos = info.pos;
    vec3 scale = info.scale;
    mat3 rot = rotationMatrix(info.rotation);
    mat3 invRot = transpose(rot);

    float hitT = 1e6;
    Hit hit;
    hit.t = -2;
    Ray newRay = ray;
    newRay.origin = invRot * (ray.origin - pos);
    newRay.dir = invRot * ray.dir;
    Ray invRay = newRay;
    invRay.dir = 1 / invRay.dir;

    Hit boxHit = intersectAABB(invRay, scaleAABB(nodes[info.numberOfNodes - 1 + info.nodeOffset].aabb, scale), 0.001, hitT);
    if (boxHit.t < 0) return hit;

    const int STACK_SIZE = 24;

    int stack[STACK_SIZE];
    int stackPtr = 0;

    int triangleOffset = info.triangleOffset;
    int nodeOffset = info.nodeOffset;
    stack[stackPtr++] = info.numberOfNodes - 1 + info.nodeOffset;

    while (stackPtr > 0) {
        int nodeIndex = stack[--stackPtr];
        BVHNode node = nodes[nodeIndex];

        Hit boxHit = intersectAABB(invRay, scaleAABB(node.aabb, scale), 0.001, hitT);
        if (boxHit.t < 0 || boxHit.t > hitT) continue;
        if (debugBVH > 0) ray.throughput *= 0.95;

        if (node.triangle >= 0) {
            bool isSmooth = info.isSmooth > 0;
            Hit triHit = triangleIntersect(scaleTri(triangles[node.triangle + triangleOffset], scale), newRay, isSmooth);
            if (triHit.t >= 0) {
                triHit.normal = normalize(rot * triHit.normal);
                if (isShadow) return triHit;
                if (triHit.t < hitT) {
                    hitT = triHit.t;
                    hit = triHit;
                }
            }
        }
        else {
            Hit leftHit; leftHit.t = -1;
            Hit rightHit; rightHit.t = -1;
            if (node.left >= 0)
                leftHit = intersectAABB(invRay, scaleAABB(node.leftAabb, scale), 0.001, hitT);
            if (node.right >= 0)
                rightHit = intersectAABB(invRay, scaleAABB(node.rightAabb, scale), 0.001, hitT);

            if (leftHit.t >= 0 && rightHit.t >= 0){
                if (leftHit.t < rightHit.t) {
                    stack[stackPtr++] = node.right + nodeOffset;
                    stack[stackPtr++] = node.left + nodeOffset;
                } else {
                    stack[stackPtr++] = node.left + nodeOffset;
                    stack[stackPtr++] = node.right + nodeOffset;
                }
            } else if (leftHit.t >= 0) stack[stackPtr++] = node.left + nodeOffset;
            else if (rightHit.t >= 0) stack[stackPtr++] = node.right + nodeOffset;
        }
    }

    //ray.origin += modelPos;
    return hit;
}

Hit rayIntersection(inout Ray ray, bool isShadow){
    Hit hit;
    hit.t = 1e6;
    Ray invRay = ray;
    invRay.dir = 1.0 / invRay.dir;
    for(int i = 0; i < numPrimitives; i += 1){
        Primitive prim = primitives[i];
        Hit newHit;
        if (prim.type == PRIM_SPHERE){
            newHit = sphereIntersect(prim, ray);
        }
        else if (prim.type == PRIM_PLANE){
            newHit = planeIntersect(prim, ray);
        }
        else if (prim.type == PRIM_CUBE){
            newHit = intersectCube(prim, ray);
        }
        else if (prim.type == PRIM_CYLINDER){
            newHit = intersectCylinder(prim, ray);
        }
        if (newHit.t > 0 && newHit.t < hit.t){
            hit = newHit;
            hit.mat = matBuffer[prim.matIndex];
            hit.primIndex = i;
        }
    }
    for (int j = 0; j < numMeshes; j += 1){
        MeshInfos info = meshInfos[j];
        Hit bvhHit = bvhIntersect(ray, info, isShadow);
        if (bvhHit.t > 0 && bvhHit.t < hit.t){
            hit = bvhHit;
            hit.mat = matBuffer[info.matIndex];
            hit.primIndex = -1;
        }
    }

    if(hit.t >= 1e6) hit.t = -1;

    return hit;
}

bool isInVolume(inout Ray ray, out Mat mat){
    Hit hit;
    hit.t = 1e6;
    Ray invRay = ray;
    invRay.dir = 1.0 / invRay.dir;
    for(int i = 0; i < numVolumesPrims; i += 1){
        Primitive prim = primitives[indicies[i + numVolumesMeshes + numLights]];
        Hit newHit;
        if (prim.type == PRIM_SPHERE){
            newHit = sphereIntersect(prim, ray);
        }
        else if (prim.type == PRIM_PLANE){
            newHit = planeIntersect(prim, ray);
        }
        else if (prim.type == PRIM_CUBE){
            newHit = intersectCube(prim, ray);
        }
        else if (prim.type == PRIM_CYLINDER){
            newHit = intersectCylinder(prim, ray);
        }
        if (newHit.t > 0 && newHit.t < hit.t){
            hit = newHit;
            hit.mat = matBuffer[prim.matIndex];
        }
    }
    for (int j = 0; j < numVolumesMeshes; j += 1){
        MeshInfos info = meshInfos[indicies[j + numLights]];
        Hit bvhHit = bvhIntersect(ray, info, false);
        if (bvhHit.t > 0 && bvhHit.t < hit.t){
            hit = bvhHit;
            hit.mat = matBuffer[info.matIndex];
        }
    }
    mat = hit.mat;
    return hit.t < 1e6 && hit.inside;
}