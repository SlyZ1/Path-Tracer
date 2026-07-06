float p_GGX_refract(vec3 wn, vec3 wm, vec3 wi, vec3 wo, float alpha, float n){
    float IdotM = dot(wi, wm);
    float OdotM = dot(wo, wm);
    float NdotM = max(EPS, dot(wn, wm));
    float D = DGTR(alpha * alpha, NdotM);
    
    float denom = IdotM + OdotM / n;
    return D * NdotM * abs(OdotM) / (denom * denom);
}

float cookTorranceDielectrics(vec3 wn, vec3 wo, vec3 wm, vec3 wi, float alpha, float n){
    float IdotM = dot(wi, wm);
    float OdotM = dot(wo, wm);
    float cosI = dot(wi, wn);
    float cosO = dot(wo, wn);
    float a2 = alpha * alpha;

    float NdotM = max(dot(wn, wm), 0.0);
    float D = DGTR(a2, NdotM);
    float G = G1GTR(a2, abs(cosO)) * G1GTR(a2, abs(cosI));

    float denom = IdotM + OdotM / n;
    float left = D * G / (denom * denom);

    float right = abs(IdotM) * abs(OdotM) / (abs(cosI) * abs(cosO));

    return left * right;
}

void glass(inout RaycastData data){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);

    if (hit.inside){
        vec3 absorption = exp(-(vec3(1) - hit.mat.color) * absorptionFactor(hit.mat) * hit.t); // Beer-Lambert
        ray.throughput *= absorption;
    }

    float n = 1 / glassIndex(hit.mat);
    vec3 normal = hit.normal;
    if (hit.inside){
        n = 1 / n;
        normal *= -1;
    }
    
    float fuzz = pbrFuzz(hit.mat);
    float alpha = fuzz * fuzz;
    vec3 wm = randomGGXHemisphere(seed, normal, alpha);
    vec3 wo = -ray.dir;

    vec3 wi = refract(ray.dir, wm, n);
    float cos1 = dot(wm, wo);
    float cos2 = dot(-wm, wi);
    bool totalReflection = n * n * (1 - cos1 * cos1) > 1;

    float reflectance = fresnel(cos1, cos2, n);
    reflectance = min(reflectance, 0.995);
    if (totalReflection || rand(seed) < reflectance){
        hit.mat.data = mData(fuzz, hit.mat.data[1]);
        updateData(data);
        metal(data);
        unwrapData(data);
        if (fuzz > EPS && n - 1 > EPS) ray.throughput /= max(reflectance, EPS);
    }
    else{
        ray.origin += hit.t * ray.dir - 0.001 * normal;
        ray.dir = wi;

        if (hit.inside && fuzz > EPS && n - 1 > EPS){
            Primitive light;
            if (numLights > 0)
                light = primitives[lightIndicies[int(rand(seed) * numLights)]];
            updateData(data);
            vec4 lightInfos = sampleLight(data, light);
            unwrapData(data);
            vec3 lightDir = lightInfos.xyz;
            float pdirect = lightInfos.w;

            Ray lightRay = ray;
            lightRay.dir = lightDir;
            if (shadow_hit(light, lightRay) > 0){
                vec3 wm_light = normalize(lightDir + wo / n);
                if (dot(wo, wm_light) * dot(lightDir, wm_light) < 0.0){
                    float pGGX = p_GGX_refract(normal, wm_light, lightDir, wo, alpha, n);
                    float weight = computeWeight(pdirect, pGGX);

                    float f_r = cookTorranceDielectrics(normal, wo, wm_light, lightDir, alpha, n);
                    float NdotL = abs(dot(lightDir, normal));
                    vec3 Le = light.mat.color * emitIntensity(light.mat);

                    ray.radiance += clamp(ray.throughput * f_r * weight * NdotL * n * n / (pdirect * (1 - reflectance)), 0.0, 1.3) * Le;
                }
            }
        }
        
        if (fuzz > EPS && n - 1 > EPS){
            float pGGX = p_GGX_refract(normal, wm, wi, wo, alpha, n);
            float f_r = cookTorranceDielectrics(normal, wo, wm, wi, alpha, n);
            float NdotL = abs(dot(wi, normal));
            ray.throughput *= f_r * NdotL * n * n / (pGGX * (1 - reflectance));
            ray.pbsdf = pGGX;
        }
    }

    if (fuzz <= EPS) ray.pbsdf = -1;

    updateData(data);
}