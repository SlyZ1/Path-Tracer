#define CONSTANT1_FON (0.5 - 2 / (3 * PI))
#define CONSTANT2_FON (2 / 3 - 28 / (15 * PI))

float p_cosineHemisphere(vec3 normal, vec3 w){
    return max(0, dot(normal, w)) / PI;
}

float cosPhiDiff(vec3 normal, vec3 lightDir, vec3 viewDir){
    vec3 Lp = normalize(lightDir - normal * dot(lightDir, normal));
    vec3 Vp = normalize(viewDir - normal * dot(viewDir, normal));

    return dot(Lp, Vp);
}

float E_FON_approx(float dotProd, float r)
{
    float mucomp = 1.0f - dotProd;
    const float g1 = 0.0571085289f;
    const float g2 = 0.491881867f;
    const float g3 = -0.332181442f;
    const float g4 = 0.0714429953f;
    float GoverPi = mucomp * (g1 + mucomp * (g2 + mucomp * (g3 + mucomp * g4)));
    return (1.0f + r * GoverPi) / (1.0f + CONSTANT1_FON * r);
}

Spectrum lambert(Spectrum spectrumValue){
    return spectrumValue / PI;
}

Spectrum oren_nayar(Spectrum spectrumValue, vec3 normal, vec3 lightDir, vec3 viewDir, float roughness){
    Spectrum lambert = lambert(spectrumValue);

    if (roughness < 1e-2) return lambert;

    float NdotL = max(dot(normal, lightDir), 0);
    float NdotV = max(dot(normal, viewDir), EPS);
    
    float A = 1 / (1 + CONSTANT1_FON * roughness);
    float B = roughness * A;

    // FON
    float cosPhi = max(0, cosPhiDiff(normal, lightDir, viewDir));
    float s2 = cosPhi * cosPhi * (1 - NdotL * NdotL) * (1 - NdotV * NdotV);
    float tq = cosPhi <= 0 ? 1 : max(NdotL, NdotV);
    Spectrum angleTerm = Spectrum(A + B * sqrt(s2) / tq);

    // EON normalization
    float EL = E_FON_approx(NdotL, roughness);
    float EV = E_FON_approx(NdotV, roughness);
    float avgE = A * (1 + CONSTANT2_FON * roughness);
    Spectrum F = spectrumValue * avgE / (Spectrum(1) - spectrumValue * (1 - avgE));
    angleTerm += F * max(1 - EL, EPS) * max(1 - EV, EPS) / max(1 - avgE, EPS);

    return lambert * angleTerm;
}

void diffuse(inout RaycastData data, bool inVolume){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);
    if (hit.inside) hit.normal *= -1;
    ray.origin += hit.t * ray.dir + 10 * EPS * hit.normal;
    ray.pbsdf = -1;

    Spectrum spectrumValue = getSpectrumValue(hit.mat);
    float orenNayarRoughness = diffuseRoughness(hit.mat);   
    vec3 viewDir = -ray.dir;
    
    // NEE
    if (!inVolume){
        Primitive light;
        Mat lightMat;
        int lightIdx = -1;
        if (numLights > 0){
            lightIdx = indicies[lightIndex(min(int(rand(seed) * numLights), numLights - 1))];
            light = primitives[lightIdx];
            lightMat = matBuffer[light.matIndex];
        }

        updateData(data);
        vec4 lightInfos = sampleLight(data, light);
        unwrapData(data);
        vec3 lightDir = lightInfos.xyz;
        float pdirect = lightInfos.w;

        Ray lightRay = ray;
        lightRay.dir = lightDir;
        if (shadow_hit(light, lightRay) > 0){
            float pbsdf = p_cosineHemisphere(hit.normal, lightDir);
            float weight = computeWeight(pdirect, pbsdf);
            float NdotL = max(dot(hit.normal, lightDir), 0);

            Spectrum lightSpectrumValue = getSpectrumValue(lightMat);
            Spectrum f_r = oren_nayar(spectrumValue, hit.normal, lightDir, viewDir, orenNayarRoughness);
            Spectrum Le = lightSpectrumValue * emitIntensity(lightMat);

            ray.radiance += clamp(ray.throughput * f_r * weight * NdotL / pdirect, 0.0, CLAMP_VAL) * Le;
        }
    }

    // BSDF sampling
    vec3 bsdfDir = randomCosineHemisphere(seed, hit.normal, 1);
    float pbsdf = p_cosineHemisphere(hit.normal, bsdfDir);
    float NdotL = max(0, dot(bsdfDir, hit.normal));
    Spectrum f_r = oren_nayar(spectrumValue, hit.normal, bsdfDir, viewDir, orenNayarRoughness);
    
    ray.throughput *= f_r * NdotL / pbsdf;
    ray.dir = bsdfDir;
    if (!inVolume) ray.pbsdf = pbsdf;

    updateData(data);
    russianRoulette(data);
}