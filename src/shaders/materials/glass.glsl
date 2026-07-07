float p_GGX_refract(vec3 N, vec3 H, vec3 L, vec3 V, float alpha, float n){
    float HdotL = dot(H, L);
    float HdotV = dot(H, V);
    float NdotV = dot(N, V);
    float NdotH = max(EPS, dot(N, H));
    float D = DVGTR(alpha * alpha, HdotV, NdotV, NdotH);
    
    float denom = HdotL + HdotV / n;
    return D * abs(HdotV) / (denom * denom);
}

float cookTorranceReflectDielectric(vec3 N, vec3 V, vec3 L, float alpha){
    float a2 = alpha * alpha;
    vec3 H = normalize(L + V);
    float NdotL = dot(N, L);
    float NdotV = dot(N, V);
    float HdotL = dot(H, L);
    float HdotV = dot(H, V);
    float NdotH = max(dot(N, H), 0.0);

    float D = DGTR(a2, NdotH);
    float G = G1GTR(a2, HdotV, NdotV) * G1GTR(a2, HdotL, NdotL);

    return D * G / max(4 * NdotV * NdotL, PROBA_EPS);
}

float cookTorranceRefractDielectric(vec3 N, vec3 V, vec3 L, float alpha, float n){
    float a2 = alpha * alpha;
    vec3 H = normalize(L + V);
    float NdotL = dot(N, L);
    float NdotV = dot(N, V);
    float HdotL = dot(H, L);
    float HdotV = dot(H, V);
    float NdotH = max(dot(N, H), 0.0);

    float D = DGTR(a2, NdotH);
    float G = G1GTR(a2, HdotV, NdotV) * G1GTR(a2, HdotL, NdotL);
    float denom = (HdotL + HdotV / n);
    float left = D * G / (denom * denom);

    float right = abs(HdotL) * abs(HdotV) / abs(NdotL) / abs(NdotV);

    return left * right;
}

float weightVNDFRefractDielectric(vec3 N, vec3 H, vec3 L, float alpha){
    float NdotL = dot(N, L);
    float HdotL = dot(H, L);
    float a2 = alpha*alpha;

    float G1_L = G1GTR(a2, HdotL, NdotL);

    return G1_L;
}

float weightVNDFReflectDielectric(vec3 N, vec3 V, vec3 L, float alpha){
    vec3 H = normalize(V + L);
    float NdotL = dot(N, L);
    float HdotL = dot(H, L);
    float a2 = alpha*alpha;

    float G1_L = G1GTR(a2, HdotL, NdotL);

    return G1_L;
}

void glass(inout RaycastData data){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);
    float n = 1 / glassIndex(hit.mat);
    vec3 N = hit.normal;
    if (hit.inside){
        n = 1 / n;
        N *= -1;
    }

    Primitive light;
    if (numLights > 0)
        light = primitives[lightIndicies[int(rand(seed) * numLights)]];
    updateData(data);
    vec4 lightInfos = sampleLight(data, light);
    unwrapData(data);
    vec3 lightDir = lightInfos.xyz;
    float pdirect = lightInfos.w;

    //Scatter
    float sigma_s = scatteringFactor(hit.mat);
    float sigma_a = absorptionFactor(hit.mat);
    if (hit.inside && sigma_s > EPS){
        float scatterDistance = -log(1 - rand(seed) * (1 - EPS)) / sigma_s;
        if (scatterDistance < hit.t - EPS){
            ray.origin += ray.dir * scatterDistance;
            ray.dir = randomOnUnitSphere(seed);
            vec3 absorption = exp(-(vec3(1) - hit.mat.color) * sigma_a * scatterDistance); // Beer-Lambert
            ray.throughput *= absorption;
            updateData(data);
            russianRoulette(data);
            return;
        }
    }

    if (hit.inside){
        vec3 absorption = exp(-(vec3(1) - hit.mat.color) * sigma_a * hit.t); // Beer-Lambert
        ray.throughput *= absorption;
    }
    
    float fuzz = pbrFuzz(hit.mat);
    float alpha = fuzz * fuzz;
    vec3 V = -ray.dir;
    vec3 H = randomGGX_VNDFHemisphere(seed, V, N, alpha);

    vec3 L = refract(ray.dir, H, n);
    float cos1 = dot(H, V);
    float cos2 = dot(-H, L);
    bool totalReflection = n * n * (1 - cos1 * cos1) > 1;

    float reflectance = fresnel(cos1, cos2, n);
    reflectance = min(reflectance, 0.995);
    if (totalReflection || rand(seed) < reflectance){
        ray.origin += hit.t * ray.dir + 0.001 * N;
        if (fuzz <= EPS || abs(n - 1) < EPS){
            vec3 newDir = reflect(ray.dir, N);
            ray.dir = newDir;
            ray.pbsdf = -1;
            updateData(data);
            return;
        }

        if (!hit.inside){
            Ray lightRay = ray;
            lightRay.dir = lightDir;
            if (shadow_hit(light, lightRay) > 0){
                float pGGX = p_VNDF_reflect(N, lightDir, V, alpha);
                float weight = computeWeight(pdirect, pGGX);

                float f_r = cookTorranceReflectDielectric(N, V, lightDir, alpha);
                float NdotL = max(dot(N, lightDir), 0.0);
                vec3 Le = light.mat.color * emitIntensity(light.mat);

                ray.radiance += clamp(ray.throughput * f_r * weight * NdotL / (pdirect), 0.0, 1.3) * Le;
            }
        }

        // BSDF sampling
        vec3 L = reflect(-V, H);
        ray.throughput *= weightVNDFReflectDielectric(N, V, L, alpha);
        ray.dir = L;
        ray.pbsdf = -1;
        if (!hit.inside) ray.pbsdf = p_VNDF_reflect(N, L, V, alpha);
    }
    else{
        ray.origin += hit.t * ray.dir - 0.001 * N;
        if (fuzz <= EPS || abs(n - 1) <= EPS){
            ray.dir = L;
            ray.pbsdf = -1;
            updateData(data);
            return;
        }

        if (hit.inside){
            Ray lightRay = ray;
            lightRay.dir = lightDir;
            if (shadow_hit(light, lightRay) > 0){
                vec3 H_light = normalize(lightDir + V / n);
                if (dot(V, H_light) * dot(lightDir, H_light) < 0.0){
                    float pGGX = p_GGX_refract(N, H_light, lightDir, V, alpha, n);
                    float weight = computeWeight(pdirect, pGGX);

                    float f_r = cookTorranceRefractDielectric(N, V, lightDir, alpha, n);
                    float NdotL = abs(dot(lightDir, N));
                    vec3 Le = light.mat.color * emitIntensity(light.mat);

                    ray.radiance += clamp(ray.throughput * f_r * weight * NdotL / pdirect, 0.0, 1.3) * Le;
                }
            }
        }
        
        ray.throughput *= weightVNDFRefractDielectric(N, H, L, alpha);
        ray.dir = L;
        ray.pbsdf = -1;
        if (hit.inside) ray.pbsdf = p_GGX_refract(N, H, L, V, alpha, n);
    }

    updateData(data);
    if (fuzz > EPS && abs(n - 1) > EPS) russianRoulette(data);
}