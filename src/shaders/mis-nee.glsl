float computeWeight(float p1, float p2){
    float n1 = p1 * p1;
    float n2 = p2 * p2;
    return n1 / (n1 + n2);
}

float shadow_hit(Primitive light, Ray ray){
    float hitSelected;
    Hit hit = rayIntersection(ray, hitSelected, true);
    if (hit.t >= 0 && isEqual(primitives[hit.primIndex], light)) return 1;
    return 0;
}

float p_direct(Primitive light, float distance, float cosLight){
    float area = 1.0f;
    switch (light.type){
        case PRIM_SPHERE:
            area = 2.0 * PI * (light.scale.x + light.scale.y + light.scale.z) / 3.0;
            break;
        case PRIM_PLANE:
            area = light.scale.x * light.scale.z;
            break;
    }
    return distance * distance / (cosLight * area * numLights + 1e-4);
}

PointAndDir sampleSphere(inout uint seed, inout Ray ray, Primitive light){
    vec3 nLight = randomOnUnitHemiphere(seed, ray.origin - light.pos);
    vec3 lightPoint = light.pos + nLight * (light.scale.x + light.scale.y + light.scale.z) / 3.0;
    return PointAndDir(lightPoint, nLight);
}

PointAndDir samplePlane(inout uint seed, inout Ray ray, Primitive light){
    mat3 rot = rotationMatrix(light.rotation);
    vec3 nLight = rot * vec3(0, 1, 0);
    if (dot(nLight, ray.origin - light.pos) < 0) nLight *= -1;

    vec3 lightPoint = (rand(seed) - 0.5) * vec3(1,0,0) * light.scale.x + (rand(seed) - 0.5) * vec3(0,0,1) * light.scale.z;
    lightPoint = rot * lightPoint + light.pos;

    return PointAndDir(lightPoint, nLight);
}

vec4 sampleLight(inout RaycastData data, Primitive light){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);
    PointAndDir pointAndNormal;
    switch (light.type) {
        case PRIM_SPHERE:
            pointAndNormal = sampleSphere(seed, ray, light);
            break;
        case PRIM_PLANE:
            pointAndNormal = samplePlane(seed, ray, light);
            break;
    }
    vec3 lightDir = normalize(pointAndNormal.point - ray.origin);
    float LdotNl = max(dot(-lightDir, pointAndNormal.dir), 0);
    
    float distance = length(pointAndNormal.point - ray.origin);
    float pdirect = p_direct(light, distance, LdotNl);

    return vec4(lightDir, pdirect);
}


void russianRoulette(inout RaycastData data){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);
#ifdef SPECTRAL
    float prob = max(max(ray.throughput.x, ray.throughput.y), max(ray.throughput.z, ray.throughput.w));
#else
    float prob = max(max(ray.throughput.x, ray.throughput.y), ray.throughput.z);
#endif
    if (rand(seed) > prob) {
        stop(hit, true);
    }  
    else { ray.throughput /= max(min(prob, 1), EPS); }

    updateData(data);
}