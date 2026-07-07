float DGTR(float a2, float NdotH){
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

float G1GTR(float a2, float HdotW, float NdotW){
    if (NdotW * HdotW <= 0.0) return 0.0;

    float NdotW2 = NdotW * NdotW;
    float tan2 = (1.0 - NdotW2) / (NdotW2);
    return 2.0 / (1.0 + sqrt(1.0 + a2 * tan2));
}

float DVGTR(float a2, float HdotV, float NdotV, float NdotH){
    if (NdotV <= 0.0) return 0.0;
    float D = DGTR(a2, NdotH);
    return G1GTR(a2, HdotV, NdotV) * max(HdotV, 0.0) * D / max(NdotV, EPS);
}

float p_VNDF_reflect(vec3 N, vec3 L, vec3 V, float alpha){
    vec3 H = normalize(L + V);
    float HdotV = dot(H, V);
    float NdotV = dot(N, V);
    float NdotH = max(dot(N, H), 0.0);

    float D = DVGTR(alpha * alpha, HdotV, NdotV, NdotH);
    return D / max(4.0 * HdotV, EPS);
}

vec3 cookTorranceMetals(Hit hit, vec3 viewDir, vec3 lightDir, float alpha){
    float a2 = alpha * alpha;
    vec3 h = normalize(lightDir + viewDir);
    float NdotL = dot(hit.normal, lightDir);
    float NdotV = dot(hit.normal, viewDir);
    float HdotL = dot(h, lightDir);
    float HdotV = dot(h, viewDir);
    float NdotH = max(dot(hit.normal, h), 0);

    float D = DGTR(a2, NdotH);
    float G = G1GTR(a2, HdotV, NdotV) * G1GTR(a2, HdotL, NdotL);
    vec3 F = schlickFresnel(HdotV, hit.mat.color);

    return F * D * G / max(4 * NdotV * NdotL, PROBA_EPS);
}

vec3 weight_VNDF_reflect(vec3 N, vec3 V, vec3 L, float alpha, vec3 albedo){
    vec3 H = normalize(V + L);
    float HdotV = dot(H, V);
    float NdotL = dot(N, L);
    float HdotL = dot(H, L);
    float a2 = alpha*alpha;

    vec3 F = schlickFresnel(max(HdotV,0.0), albedo);
    float G1_wi = G1GTR(a2, HdotL, NdotL);

    return F * G1_wi;
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
        updateData(data);
        return;
    }

    // MIS
    Primitive light;
    if (numLights > 0)
        light = primitives[lightIndicies[int(rand(seed) * numLights)]];
    float fuzz = pbrFuzz(hit.mat);
    float alpha = fuzz * fuzz;
    vec3 V = -ray.dir;
    vec3 N = hit.normal;

    // Direct lighting
    updateData(data);
    vec4 lightInfos = sampleLight(data, light);
    unwrapData(data);
    vec3 L = lightInfos.xyz;
    float pdirect = lightInfos.w;

    Ray lightRay = ray;
    lightRay.dir = L;
    if (shadow_hit(light, lightRay) > 0){
        float pGGX = p_VNDF_reflect(N, L, V, alpha);
        float weight = computeWeight(pdirect, pGGX);

        vec3 f_r = cookTorranceMetals(hit, V, L, alpha);
        float NdotL = max(dot(N, L), 0);
        vec3 Le = light.mat.color * emitIntensity(light.mat);

        ray.radiance += clamp(ray.throughput * f_r * weight * NdotL / pdirect, 0.0, 1.3) * Le;
    }

    // BSDF sampling
    vec3 H = randomGGX_VNDFHemisphere(seed, V, N, alpha);
    L = reflect(-V, H);
    ray.throughput *= weight_VNDF_reflect(N, V, L, alpha, hit.mat.color);
    ray.dir = L;
    ray.pbsdf = p_VNDF_reflect(N, L, V, alpha);

    updateData(data);
    russianRoulette(data);
}