float computeWeight(float p1, float p2){
    float n1 = p1 * p1;
    float n2 = p2 * p2;
    return n1 / (n1 + n2);
}

float ellipsoidVisibleAreaApprox(vec3 scale){
    const float p = 1.6075;
    float ap = pow(scale.x, p);
    float bp = pow(scale.y, p);
    float cp = pow(scale.z, p);
    float totalArea = 4.0 * PI * pow((ap*bp + bp*cp + cp*ap) / 3.0, 1.0/p);
    return totalArea * 0.5;
}

float p_direct(Primitive light, float distance, float cosLight){
    float area = ellipsoidVisibleAreaApprox(light.scale);
    return distance * distance / (cosLight * area * numLights + 1e-4);
}

float shadow_hit(Primitive light, Ray ray){
    Hit hit = rayIntersection(ray, true);
    if (hit.t >= 0 && isEqual(primitives[hit.primIndex], light)) return 1;
    return 0;
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

vec4 sampleLight(inout RaycastData data, Primitive light){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);

    vec3 nLight = randomOnUnitHemiphere(seed, ray.origin - light.pos);
    vec3 lightPoint = light.pos + nLight * light.scale;
    vec3 lightDir = normalize(lightPoint - ray.origin);
    float LdotNl = max(dot(-lightDir, nLight), 0);
    
    float distance = length(lightPoint - ray.origin);
    float pdirect = p_direct(light, distance, LdotNl);

    updateData(data);
    return vec4(lightDir, pdirect);
}