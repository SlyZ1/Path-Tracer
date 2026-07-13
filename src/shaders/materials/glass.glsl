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

#ifdef SPECTRAL
bool checkReflection(inout Ray ray, inout uint seed, inout SpectralParam n, vec3 H, float cos1, float dispertionFactor){
    vec3 LX = refract(ray.dir, H, n.x);
    vec3 LY = refract(ray.dir, H, n.y);
    vec3 LZ = refract(ray.dir, H, n.z);
    vec3 LW = refract(ray.dir, H, n.w);
    Spectrum cos2 = vec4(dot(-H, LX), dot(-H, LY), dot(-H, LZ), dot(-H, LW));
    bool totalReflectionX = n.x * n.x * (1 - cos1 * cos1) > 1;
    bool totalReflectionY = n.y * n.y * (1 - cos1 * cos1) > 1;
    bool totalReflectionZ = n.z * n.z * (1 - cos1 * cos1) > 1;
    bool totalReflectionW = n.w * n.w * (1 - cos1 * cos1) > 1;

    SpectralParam reflectance = fresnel(cos1, cos2, n);
    reflectance = min(reflectance, Spectrum(0.995));
    float randomReflectance = rand(seed);
    bool reflectX = totalReflectionX || randomReflectance < reflectance.x;
    bool reflectY = totalReflectionY || randomReflectance < reflectance.y;
    bool reflectZ = totalReflectionZ || randomReflectance < reflectance.z;
    bool reflectW = totalReflectionW || randomReflectance < reflectance.w;
    if (!(reflectX && reflectY && reflectZ && reflectW) 
    && (ray.throughput.y > 0 || ray.throughput.z > 0 || ray.throughput.w > 0)
    && dispertionFactor > EPS)
    {
        ray.throughput *= Spectrum(4, 0, 0, 0);
        ray.lambda = Spectrum(ray.lambda.x);
        n = SpectralParam(n.x);
    }
    return reflectX;
}
bool IORcloseToOne(SpectralParam n){
    return abs(min(min(n.x, n.y), min(n.z, n.w)) - 1) < EPS;
}
#else
bool checkReflection(inout Ray ray, inout uint seed, SpectralParam n, vec3 H, float cos1, float dispertionFactor){
    vec3 L = refract(ray.dir, H, n);
    float cos2 = dot(-H, L);
    bool totalReflection = n * n * (1 - cos1 * cos1) > 1;

    SpectralParam reflectance = fresnel(cos1, cos2, n);
    reflectance = min(reflectance, 0.995);
    return totalReflection || rand(seed) < reflectance;
}
bool IORcloseToOne(SpectralParam n){
    return abs(n - 1) < EPS;
}
#endif

void glass(inout RaycastData data){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);
    ray.pbsdf = -1;

    Spectrum spectrumValue = getSpectrumValue(hit.mat);
    SpectralParam glassN = SpectralParam(glassIndex(hit.mat));
#ifdef SPECTRAL
    vec4 scaledLambda = ray.lambda / 1000.0;
    glassN += SpectralParam(dispertionFactor(hit.mat)) / (scaledLambda * scaledLambda);
#endif
    SpectralParam n = 1.0 / glassN;
    vec3 N = hit.normal;
    if (hit.inside){
        n = 1 / n;
        N *= -1;
    }

    Primitive light;
    Mat lightMat;
    int lightIndex = -1;
    if (numLights > 0){
        lightIndex = lightIndicies[int(rand(seed) * numLights)];
        light = primitives[lightIndex];
        lightMat = matBuffer[light.matIndex];
    }
    updateData(data);
    vec4 lightInfos = sampleLight(data, light);
    unwrapData(data);
    vec3 lightDir = lightInfos.xyz;
    float pdirect = lightInfos.w;
    
    float sigma_s = scatteringFactor(hit.mat);
    float sigma_a = absorptionFactor(hit.mat);
    if (hit.inside && sigma_s < EPS){
        Spectrum absorption = exp(-(Spectrum(1.0) - spectrumValue) * sigma_a * hit.t); // Beer-Lambert
        ray.throughput *= absorption;
    }
    
    float fuzz = pbrFuzz(hit.mat);
    float alpha = fuzz * fuzz;
    vec3 V = -ray.dir;
    vec3 H = randomGGX_VNDFHemisphere(seed, V, N, alpha);
    float cos1 = dot(V, H);

    bool reflects = checkReflection(ray, seed, n, H, cos1, dispertionFactor(hit.mat));
    if (reflects){
        ray.origin += hit.t * ray.dir + 0.001 * N;
        if (fuzz <= EPS || IORcloseToOne(n)){
            vec3 newDir = reflect(ray.dir, N);
            ray.dir = newDir;
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
                Spectrum Le = getSpectrumValue(lightMat) * emitIntensity(lightMat);

                ray.radiance += clamp(ray.throughput * f_r * weight * NdotL / (pdirect), 0.0, CLAMP_VAL) * Le;
            }
        }

        // BSDF sampling
        vec3 L = reflect(-V, H);
        ray.throughput *= weightVNDFReflectDielectric(N, V, L, alpha);
        ray.dir = L;
        if (!hit.inside){
            ray.pbsdf = p_VNDF_reflect(N, L, V, alpha);
        }
    }
    else{
        ray.origin += hit.t * ray.dir - 0.001 * N;
        float IOR = paramToFloat(n);
        if (fuzz <= EPS || IORcloseToOne(n)){
            ray.dir = refract(ray.dir, N, IOR);
            updateData(data);
            return;
        }

        if (hit.inside){
            Ray lightRay = ray;
            lightRay.dir = lightDir;
            if (shadow_hit(light, lightRay) > 0){
                vec3 H_light = normalize(lightDir + V / IOR);
                if (dot(V, H_light) * dot(lightDir, H_light) < 0.0){
                    float pGGX = p_GGX_refract(N, H_light, lightDir, V, alpha, IOR);
                    float weight = computeWeight(pdirect, pGGX);
                    float f_r = cookTorranceRefractDielectric(N, V, lightDir, alpha, IOR);
                    float NdotL = abs(dot(lightDir, N));
                    Spectrum Le = getSpectrumValue(lightMat) * emitIntensity(lightMat);

                    ray.radiance += clamp(ray.throughput * f_r * weight * NdotL / pdirect, 0.0, CLAMP_VAL) * Le;
                }
            }
        }
        
        vec3 L = refract(ray.dir, H, IOR);
        ray.throughput *= weightVNDFRefractDielectric(N, H, L, alpha);
        ray.dir = L;
        if (hit.inside){
            ray.pbsdf = p_GGX_refract(N, H, L, V, alpha, IOR);
        }
    }
    updateData(data);
    if (fuzz > EPS && IORcloseToOne(n)) russianRoulette(data);
}