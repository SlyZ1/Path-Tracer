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

    return G1_L; // Add fresnel
}

#ifdef SPECTRAL
bool checkReflection(inout RaycastData data, vec3 H, float cos1, float dispertionFactor, inout FresnelDielectricParams params){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);

    SpectralParam glassN = hit.inside ? params.n : 1.0 / params.n;
    SpectralParam reflectance = airyDielectric(cos1, filmIOR(params.mat), filmDepth(params.mat), params.lambda, glassN, hit.inside);
    reflectance = min(reflectance, Spectrum(0.995));

    bvec4 totalReflectionAll = bvec4(greaterThan(params.n * params.n * (1 - cos1 * cos1), vec4(1)));
    vec4 randomAll = vec4(rand(seed), rand(seed), rand(seed), rand(seed));
    bvec4 stochasticReflect = bvec4(lessThan(randomAll, reflectance));
    bvec4 reflectAll = bvec4(
        stochasticReflect.x || totalReflectionAll.x,
        stochasticReflect.y || totalReflectionAll.y,
        stochasticReflect.z || totalReflectionAll.z,
        stochasticReflect.w || totalReflectionAll.w
    );

    if (ray.throughput.y > 0 || ray.throughput.z > 0 || ray.throughput.w > 0){
        bool pathDiverge = !all(reflectAll) && !all(!reflectAll);
        bool disperse = all(!reflectAll) && dispertionFactor > 0;
        if (pathDiverge || disperse)
        {
            ray.throughput *= Spectrum(4, 0, 0, 0);
            ray.lambda = Spectrum(ray.lambda.x);
            params.n = SpectralParam(params.n.x);
        }
    }
    if (reflectAll.x)
        ray.throughput *= reflectance / reflectance.x;
    else
        ray.throughput *= (SpectralParam(1) - reflectance) / (1 - reflectance.x);
    updateData(data);
    return reflectAll.x;
}

#else
bool checkReflection(inout RaycastData data, vec3 H, float cos1, float dispertionFactor, inout FresnelDielectricParams params){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);

    vec3 L = refract(ray.dir, H, params.n);
    SpectralParam cos2 = dot(-H, L);
    bool totalReflection = params.n * params.n * (1 - cos1 * cos1) > 1;

    SpectralParam reflectance = fresnel(cos1, cos2, params.n);
    reflectance = min(reflectance, 0.995);
    bool result = totalReflection || rand(seed) < reflectance;

    updateData(data);
    return result;
}
#endif

bool IORcloseToOne(SpectralParam n){
    return any(lessThan(abs(vec4(n) - vec4(1.0)), vec4(EPS)));
}

SpectralParam computeIOR(inout RaycastData data){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);

    SpectralParam glassN = SpectralParam(glassIndex(hit.mat));
#ifdef SPECTRAL
    vec4 scaledLambda = ray.lambda / 1000.0;
    glassN += SpectralParam(dispertionFactor(hit.mat)) / (scaledLambda * scaledLambda);
#endif
    SpectralParam n = 1.0 / glassN;
    if (hit.inside){
        n = 1 / n;
    }

    return n;
}

void glass(inout RaycastData data, bool inVolume){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);
    ray.pbsdf = -1;

    Spectrum spectrumValue = getSpectrumValue(hit.mat);
    SpectralParam n = computeIOR(data);
    vec3 N = hit.inside ? -hit.normal : hit.normal;
    FresnelDielectricParams fresnelParams = newFresnelDielectricParams(n);
    
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

    updateData(data);
    bool reflects = checkReflection(data, H, cos1, dispertionFactor(hit.mat), fresnelParams);
    unwrapData(data);
    n = fresnelParams.n;
    if (reflects){
        ray.origin += hit.t * ray.dir + 10 * EPS * N;
        if (fuzz <= EPS || IORcloseToOne(n)){
            vec3 newDir = reflect(ray.dir, N);
            ray.dir = newDir;
            updateData(data);
            return;
        }

        // BSDF sampling
        vec3 L = reflect(-V, H);
        ray.throughput *= weightVNDFReflectDielectric(N, V, L, alpha);
        ray.dir = L;
    }
    else{
        ray.origin += hit.t * ray.dir - 10 * EPS * N;
        float IOR = paramToFloat(n);
        if (fuzz <= EPS || IORcloseToOne(n)){
            ray.dir = refract(ray.dir, N, IOR);
            updateData(data);
            return;
        }

        vec3 L = refract(ray.dir, H, IOR);
        ray.throughput *= weightVNDFRefractDielectric(N, H, L, alpha);
        ray.dir = L;
    }
    updateData(data);
    if (fuzz > EPS && IORcloseToOne(n)) russianRoulette(data);
}