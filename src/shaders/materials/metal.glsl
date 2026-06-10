float DGTR(float a2, float NdotH){
    float denom = NdotH * NdotH * (a2 - 1) + 1;
    return a2 / (PI * denom * denom);
}

float p_GGX(vec3 normal, vec3 l, vec3 v, float alpha, bool simplify){
    vec3 h = normalize(l + v);
    float NdotH = max(dot(normal, h), EPS);
    float VdotH = max(dot(v, h), 0.0);
    float D = 1;
    if (!simplify) D = DGTR(alpha * alpha, NdotH);
    return D * NdotH / (4.0 * VdotH);
}

float G1GTR(float a2, float NdotW){
    if (NdotW <= 0.0) return 0.0;

    float NdotW2 = NdotW * NdotW;
    float tan2 = (1.0 - NdotW2) / (NdotW2);
    return 2.0 / (1.0 + sqrt(1.0 + a2 * tan2));
}

vec3 cookTorrance(Hit hit, vec3 viewDir, vec3 lightDir, float alpha, bool simplify){
    vec3 h = normalize(lightDir + viewDir);
    float NdotL = max(dot(hit.normal, lightDir), 0);
    float NdotV = max(dot(viewDir, hit.normal), 0);
    float VdotH = max(dot(h, viewDir), 0);
    float a2 = alpha * alpha;
    float D = 1; // D is simplified with the PDF
    if (!simplify){
        float NdotH = max(dot(hit.normal, h), 0);
        D = DGTR(a2, NdotH);
    }
    float G = G1GTR(a2, NdotV) * G1GTR(a2, NdotL);
    vec3 F = schlickFresnel(VdotH, hit.mat.color);

    return F * D * G / max(4 * NdotV * (simplify ? 1 : NdotL), PROBA_EPS);
}

void metal(inout RaycastData data){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);
    ray.origin += hit.t * ray.dir + hit.normal * EPS;
    
    if (pbrFuzz(hit.mat) < EPS){
        vec3 newDir = reflect(ray.dir, hit.normal);
        float VdotN = max(dot(-ray.dir, hit.normal), 0.0);
        vec3 f_r = schlickFresnel(VdotN, hit.mat.color);
        ray.throughput *= f_r;
        ray.dir = newDir;
        ray.pbsdf = -1;
        data = RaycastData(hit, ray, seed);
        return;
    }

    // MIS
    Primitive light;
    if (numLights > 0)
        light = primitives[lightIndicies[int(rand(seed) * numLights)]];
    float fuzz = pbrFuzz(hit.mat);
    float alpha = fuzz * fuzz;
    vec3 viewDir = -ray.dir;

    // Direct lighting
    updateData(data);
    vec4 lightInfos = sampleLight(data, light);
    unwrapData(data);
    vec3 lightDir = lightInfos.xyz;
    float pdirect = lightInfos.w;

    Ray lightRay = ray;
    lightRay.dir = lightDir;
    if (shadow_hit(light, lightRay) > 0){
        float pGGX = p_GGX(hit.normal, lightDir, viewDir, alpha, false);
        float weight = computeWeight(pdirect, pGGX);

        vec3 f_r = cookTorrance(hit, viewDir, lightDir, alpha, false);
        float NdotL = max(dot(hit.normal, lightDir), 0);
        vec3 Le = light.mat.color * emitIntensity(light.mat);

        ray.radiance += clamp(ray.throughput * f_r * weight * NdotL / pdirect, 0.0, 1.3) * Le;
    }

    // BSDF sampling
    vec3 h = randomGGXHemisphere(seed, hit.normal, alpha);
    vec3 GGXDir = reflect(-viewDir, h);
    float pGGX = p_GGX(hit.normal, GGXDir, viewDir, alpha, false);
    float NdotL = max(0, dot(GGXDir, hit.normal));
    vec3 f_r = cookTorrance(hit, viewDir, GGXDir, alpha, false);
    ray.throughput *= f_r * NdotL / pGGX;
    ray.dir = GGXDir;
    ray.pbsdf = pGGX;

    updateData(data);
    russianRoulette(data);
}