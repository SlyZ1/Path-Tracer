
float DGTR(float a2, float NdotH){
    float denom = NdotH * NdotH * (a2 - 1) + 1;
    return a2 / max(PI * denom * denom, PROBA_EPS);
}

float p_GGX(vec3 normal, vec3 l, vec3 v, float alpha, bool simplify){
    vec3 h = normalize(l + v);
    float NdotH = max(dot(normal, h), 0.0);
    float VdotH = max(dot(v, h), 0.0);
    float D = 1;
    if (!simplify) D = DGTR(alpha * alpha, NdotH);
    return D * NdotH / max(4.0 * VdotH, PROBA_EPS);
}

float G1GTR(float a2, float NdotW){
    if (NdotW <= 0.0) return 0.0;

    float NdotW2 = NdotW * NdotW;
    float tan2 = (1.0 - NdotW2) / max(NdotW2, PROBA_EPS);
    return 2.0 / (1.0 + sqrt(1.0 + a2 * tan2));
}

vec3 cookTorrance(Hit hit, vec3 viewDir, vec3 lightDir, float alpha, bool simplify){
    vec3 h = normalize(lightDir + viewDir);
    float NdotL = max(dot(hit.normal, lightDir), 0);
    float NdotV = max(dot(viewDir, hit.normal), 0);
    float VdotH = max(dot(h, viewDir), EPS);
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

void metal(World world, inout RaycastData data){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);
    Light light;
#if NUM_LIGHT > 0
    light = world.lights[int(rand(seed) * NUM_LIGHT)];
#endif
    ray.origin += hit.t * ray.dir + hit.normal * EPS;
    
    if (pbrFuzz(hit.mat) < EPS){
        vec3 newDir = reflect(ray.dir, hit.normal);
        float VdotN = max(dot(-ray.dir, hit.normal), 0.0);
        vec3 f_r = schlickFresnel(VdotN, hit.mat.color);
        ray.throughput *= f_r;
        ray.dir = newDir;
        data = RaycastData(hit, ray, seed);
        return;
    }

    // MIS
    float fuzz = pbrFuzz(hit.mat);
    float alpha = fuzz * fuzz;
    float wdirect = 0.3 * alpha;
    float wGGX = 1 - wdirect;
    wdirect = NUM_LIGHT > 0 ? wdirect : 0;
    wGGX = NUM_LIGHT > 0 ? wGGX : 1;
    float r = rand(seed);
    if (r < wGGX){
        vec3 viewDir = -ray.dir;
        vec3 h = randomGGXHemisphere(seed, hit.normal, alpha);
        vec3 newDir = mix(reflect(-viewDir, h), hit.normal, 0.0);
        
        float pGGX = p_GGX(hit.normal, newDir, viewDir, alpha, true);
        vec3 f_r = cookTorrance(hit, viewDir, newDir, alpha, true);
        ray.throughput *= f_r /* * NdotL (but is in cookTorrance) */ / (pGGX * wGGX);
        ray.dir = newDir;

        Hit nextHit = rayIntersection(world, ray);
        if (nextHit.t > 0 && nextHit.mat.type == MAT_EMIT){
            vec3 Le = nextHit.mat.color * emitIntensity(nextHit.mat);
            float LdotNl = max(dot(-newDir, nextHit.normal), 0);
            float pdirect = p_direct(light, nextHit.t, LdotNl)
                            * shadow_hit(light, world, ray);
            float weight = wGGX * pGGX / (wGGX * pGGX + wdirect * pdirect);

            ray.radiance += clamp(ray.throughput * weight, 0.0, 1.5) * Le;
            stop(hit, true);
        }
    }
    else{
        // Direct lighting
        vec3 viewDir = -ray.dir;
        updateData(data);
        float pdirect = sampleLight(data, light);
        unwrapData(data);
        vec3 f_r = cookTorrance(hit, viewDir, ray.dir, alpha, false);
        ray.throughput *= f_r;

        if (shadow_hit(light, world, ray) > 0){
            float pGGX = p_GGX(hit.normal, ray.dir, viewDir, alpha, false);
            float weight = 1.0 / (wdirect * pdirect + wGGX * pGGX);
            vec3 Le = light.color * light.intensity;
            ray.radiance += clamp(ray.throughput * weight, 0.0, 1.5) * Le;
            stop(hit, true);
        }
        else{
            ray.throughput /= wdirect * pdirect;
            stop(hit, false);
        }
    }

    updateData(data);
    russianRoulette(data);
}