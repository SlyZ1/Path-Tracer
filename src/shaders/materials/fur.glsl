Spectrum computeT(Spectrum sigmaA, float h, float eta, float cosThetaT){
    float sinGammaT = h / eta;
    float cosGammaT = sqrt(1 - sinGammaT * sinGammaT);
    return exp(-2 * sigmaA * cosGammaT / cosThetaT);
}

Spectrum computeA(float p, float h, float eta, float cosThetaD, float cosThetaT, Spectrum sigmaAPrime){
    float n = 1.0 / eta;
    float cos1 = cosThetaD * sqrt(1 - h * h);
    float f = fresnel(cos1, computeCos2(cos1, n), n);
    if (p == 0){
        return Spectrum(f);
    }
    else{
        Spectrum T = computeT(sigmaAPrime, h, eta, cosThetaT);
        Spectrum right = T;
        if (p == 2) right *= f * T;
        return (1 - f) * (1 - f) * right;
    }
}

float computeEtaPrime(float eta, float cosThetaD){
    float sin2ThetaD = 1 - cosThetaD * cosThetaD;
    return sqrt(eta * eta - sin2ThetaD) / cosThetaD;
}

#define ALPHA PI * 5.0 / 180.0 // 5° in rad
float computeThetaCone(float thetaI, float p){
    if (p == 0) return -thetaI + 2 * ALPHA;
    else if (p == 1) return -thetaI - ALPHA;
    else return -thetaI - 4 * ALPHA;
}

float computeLobeVariance(float vR, float p){
    if (p == 0) return vR;
    if (p == 1) return vR / 4.0; // variance = sigma², 2 -> 4
    if (p == 2) return vR * 4.0;
}

float sampleSphericalGaussian(inout uint seed, float v){
    float x = rand(seed);
    float expTerm = exp(-2.0 / v);
    float val = (1.0 - x) + x * expTerm;
    float u = 1.0 + v * log(max(val, 1e-12));
    return clamp(u, -1.0, 1.0);
}

float sampleThetaOut(inout uint seed, float u, float thetaCone){
    u = clamp(u, -1.0, 1.0);
    float x = rand(seed);
    float thetaPrime = PI * 0.5 - thetaCone;
    return asin(u * cos(thetaPrime) + sqrt(1 - u * u) * cos(2 * PI * x) * sin(thetaPrime));
}

vec2 sampleP(inout uint seed, float A0, float A1, float A2){
    float sum = A0 + A1 + A2 + 1e-6;
    float w0 = A0 / sum;
    float w1 = A1 / sum;
    float x = rand(seed);
    if (x < w0) return vec2(0, w0);
    else if (x < w0 + w1) return vec2(1, w1);
    else return vec2(2, A2 / sum);
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

float computePhiOut(int p, float gamma_i, float gamma_t){
    return p * (2 * gamma_t + PI) - 2 * gamma_i;
}

void fur(inout RaycastData data, bool inVolume){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);
    ray.pbsdf = -1;
    ray.origin += ray.dir * hit.t;

    float h = rand(seed) * 2 - 1;

    vec3 wi = -ray.dir;
    vec3 normal = hit.normal;
    normal = normalize(normal + hit.tangent * dot(hit.tangent, normal));
    vec3 bitangent = cross(hit.tangent, normal);

    float thetaI, phiI, sinThetaI, cosThetaI;
    computeThetaPhi(wi, hit.tangent, bitangent, normal, thetaI, phiI, sinThetaI, cosThetaI);

    const float eta = 1.55;
    Spectrum sigmaA = (Spectrum(1.0) - getSpectrumValue(hit.mat)) * 0.5;
    float sinThetaT = sinThetaI / eta;
    float cosThetaT = sqrt(1 - sinThetaT * sinThetaT);
    Spectrum sigmaAPrime = sigmaA / cosThetaT;

    
    //Compute Aspec0 - R
    int p = 0;
    float thetaC = computeThetaCone(thetaI, p);
    float cosThetaD = cos(thetaC - ALPHA); // No roughness, perfect relfection
    float etaPrime = computeEtaPrime(eta, cosThetaD);
    Spectrum Aspec0 = computeA(p, h, etaPrime, cosThetaD, cosThetaT, sigmaAPrime);
    
    // Compute Aspec1 - TT
    p = 1;
    thetaC = computeThetaCone(thetaI, p);
    cosThetaD = cos(thetaC + ALPHA / 2.0); // No roughness, perfect transmission
    etaPrime = computeEtaPrime(eta, cosThetaD);
    Spectrum Aspec1 = computeA(p, h, etaPrime, cosThetaD, cosThetaT, sigmaAPrime);
    
    // Compute Aspec2 - TRT
    p = 2;
    thetaC = computeThetaCone(thetaI, p);
    cosThetaD = cos(thetaC + ALPHA * 2.0); // No roughness, perfect relfection
    etaPrime = computeEtaPrime(eta, cosThetaD);
    Spectrum Aspec2 = computeA(p, h, etaPrime, cosThetaD, cosThetaT, sigmaAPrime);

    vec2 pSample = sampleP(seed, luminanceMean(Aspec0), luminanceMean(Aspec1), luminanceMean(Aspec2));
    p = int(pSample.x);
    float wp = pSample.y;

    thetaC = computeThetaCone(thetaI, p);
    float betaR = PI * betaM(hit.mat) / 180.0;
    float u = sampleSphericalGaussian(seed, computeLobeVariance(betaR * betaR, p));
    float thetaO = sampleThetaOut(seed, u, thetaC);
    float thetaD = (thetaO - thetaI) / 2.0;
    cosThetaD = cos(thetaD);
    etaPrime = computeEtaPrime(eta, cosThetaD);
    
    float gamma_i = asin(h);
    float gamma_t = asin(h / etaPrime);
    float deltaPhi = sampleLogit(rand(seed), 1.0, -PI, PI);
    float phiO = computePhiOut(p, gamma_i, gamma_t) + deltaPhi * PI * betaN(hit.mat) / 180.0;
    
    Spectrum Ap = computeA(p, h, etaPrime, cosThetaD, cosThetaT, sigmaAPrime);
    Spectrum weight = Ap / wp;

    ray.throughput *= weight;
    ray.dir = directionFromThetaPhi(thetaO, phiO, hit.tangent, normal, bitangent);
    if (dot(ray.dir, hit.normal) > 0){
        ray.origin += normal * EPS;
    }
    else{
        ray.origin -= normal * 0.0007 * 2;
    }

    updateData(data);
    russianRoulette(data);
}