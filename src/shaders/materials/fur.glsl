#define ETA 1.55

Spectrum computeT(Spectrum sigmaA, float h, float etaP, float sinThetaI){
    float sinThetaT = sinThetaI / ETA;
    float cosThetaT = sqrt(1 - sinThetaT * sinThetaT);

    float sinGammaT = h / etaP;
    float cosGammaT = sqrt(1 - sinGammaT * sinGammaT);
    return exp(-2 * sigmaA * (cosGammaT / cosThetaT));
}

Spectrum computeA(int p, float h, float cosTheta, float cosThetaI, Spectrum T){
    float cos1 = cosThetaI * sqrt(1 - h * h);
    float f = fresnel(cos1, computeCos2(cos1, ETA), ETA);
    if (p == 0) return Spectrum(f); 
    
    Spectrum res = (1 - f) * (1 - f) * T;
    if (p >= 2) res *= f * T;
    if (p == 3) res *= f * T / max(1.0 - f * T, 0.05);
    return res;
}

float computeEtaPrime(float cosThetaI){
    float sin2ThetaI = 1 - cosThetaI * cosThetaI;
    return sqrt(ETA * ETA - sin2ThetaI) / cosThetaI;
}

float computeThetaCone(float thetaI, int p, float alpha){
    if (p == 0) return -thetaI + 2 * alpha;
    else if (p == 1) return -thetaI - alpha;
    else return -thetaI - 4 * alpha;
}

float computeLobeVariance(float vR, int p){
    if (p == 0) return vR;
    if (p == 1) return vR * 0.25; // variance = sigma², 2 -> 4
    return vR * 4.0;
}

float reparamBetaM(float bM){
    return 0.726 * bM + 0.812 * bM * bM + 3.7 * powi(bM, 20);
}

float reparamBetaN(float bN){
    return 0.265 * bN + 1.194 * bN * bN + 5.372 * powi(bN, 22);
}

float sampleThetaOut(inout uint seed, float v, float thetaCone){
    float x1 = rand(seed);
    float x2 = rand(seed);
    float val = max(x1, 1e-5) + (1.0 - x1) * exp(-2.0 / (v + EPS));
    float u = 1.0 + v * log(val);
    u = clamp(u, -1.0, 1.0);
    float theta = 0.5 * PI - thetaCone;
    float s = u * cos(theta) + sqrt(1 - u * u) * cos(2 * PI * x2) * sin(theta);
    return asin(clamp(s, -1.0, 1.0));
}

vec2 sampleP(inout uint seed, vec4 Aspec){
    float sum = Aspec.x + Aspec.y + Aspec.z + Aspec.w + 1e-6;
    float w0 = Aspec.x / sum;
    float w1 = Aspec.y / sum;
    float w2 = Aspec.z / sum;
    float w3 = Aspec.w / sum;
    float x = rand(seed);
    if (x < w0) return vec2(0, w0);
    else if (x < w0 + w1) return vec2(1, w1);
    else if (x < w0 + w1 + w2) return vec2(2, w2);
    else return vec2(3, w3);
}

void computeThetaPhi(vec3 wi, vec3 tangent, vec3 bitangent, vec3 normal, out float thetaI, out float phiI, out float sinThetaI, out float cosThetaI){
    sinThetaI = dot(wi, tangent);
    cosThetaI = sqrt(max(0.0, 1.0 - sinThetaI*sinThetaI));
    thetaI = asin(clamp(sinThetaI, -1.0, 1.0));
    
    float projNormal = dot(wi, normal);
    float projBitangent = dot(wi, bitangent);
    phiI = atan(projBitangent, projNormal);
}

vec3 directionFromThetaPhi(float theta, float phi, vec3 tangent, vec3 normal, vec3 bitangent){
    float sinTheta = sin(theta);
    float cosTheta = cos(theta);
    
    return sinTheta * tangent + cosTheta * cos(phi) * normal + cosTheta * sin(phi) * bitangent;
}

float computePhiOut(int p, float gammaI, float gammaT){
    return p * (2 * gammaT + PI) - 2 * gammaI;
}

vec4 computeAspec(float cosThetaI, float h, float cosTheta, Spectrum T){
    Spectrum Aspec0 = computeA(0, h, cosTheta, cosThetaI, T);
    Spectrum Aspec1 = computeA(1, h, cosTheta, cosThetaI, T);
    Spectrum Aspec2 = computeA(2, h, cosTheta, cosThetaI, T);
    Spectrum Aspec3 = computeA(3, h, cosTheta, cosThetaI, T);

    return vec4(luminanceMean(Aspec0), luminanceMean(Aspec1), luminanceMean(Aspec2), luminanceMean(Aspec3));
}

Spectrum computeSigmaA(Spectrum albedo, float betaN){
    float betaN2 = betaN * betaN;
    float betaN3 = betaN2 * betaN;
    float pol = 5.969 - 0.215 * betaN + 2.532 * betaN2 - 10.73 * betaN3 + 5.574 * betaN2 * betaN2 + 0.245 * betaN2 * betaN3;
    Spectrum res = log(albedo) / pol;
    return res * res;
}

void fur(inout RaycastData data, bool inVolume){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);
    ray.pbsdf = -1;
    ray.origin += ray.dir * hit.t;

    // 1
    vec3 wi = -ray.dir;
    vec3 normal = hit.normal;
    vec3 tangent = hit.tangent;
    normal = normalize(normal - tangent * dot(tangent, normal));
    vec3 bitangent = cross(tangent, normal);

    float thetaI, phiI, sinThetaI, cosThetaI;
    computeThetaPhi(wi, tangent, bitangent, normal, thetaI, phiI, sinThetaI, cosThetaI);

    vec3 azimuthalDir = normalize(wi - tangent * dot(tangent, wi));
    float h = dot(cross(normal, azimuthalDir), tangent);

    // 2
    Spectrum sigmaA = (1.0 - getSpectrumValue(hit.mat)) * 0.5;
    float etaPrime = computeEtaPrime(cosThetaI);
    Spectrum T = computeT(sigmaA, h, etaPrime, sinThetaI);

    // 3 - sample p
    vec4 Aspec = computeAspec(cosThetaI, h, dot(wi, normal), T);
    vec2 pSample = sampleP(seed, Aspec);
    int p = int(pSample.x);
    float wp = pSample.y;
    
    // 4 compute
    float thetaC = computeThetaCone(thetaI, p, PI * alpha(hit.mat) / 180.0);
    float betaR = reparamBetaM(betaM(hit.mat));
    float v = computeLobeVariance(betaR * betaR, p);
    float thetaO = sampleThetaOut(seed, v, thetaC);

    Spectrum Ap = computeA(p, h, dot(wi, normal), cosThetaI, T);
    ray.throughput *= Ap / (wp);
    
    float phiO;
    if (p < 3){
        float gammaI = asin(h);
        float gammaT = asin(h / etaPrime);
        float repamBetaN = reparamBetaN(betaN(hit.mat));
        float deltaPhi = sampleLogit(rand(seed), repamBetaN, -PI, PI);
        phiO = phiI + computePhiOut(p, gammaI, gammaT) + deltaPhi;
        // while (phiO < -PI) phiO += 2 * PI;
        // while (phiO > PI) phiO -= 2 * PI;
    }
    else{
        phiO = -phiI + 2.0 * PI * rand(seed);
    }

    ray.dir = directionFromThetaPhi(thetaO, phiO, tangent, normal, bitangent);
    if (dot(ray.dir, hit.normal) > 0){
        ray.origin += hit.normal * 0.0001;
    }
    else{
        ray.origin -= hit.normal * 0.0001;
    }

    updateData(data);
    russianRoulette(data);
}