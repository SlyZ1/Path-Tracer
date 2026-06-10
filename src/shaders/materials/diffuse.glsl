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

vec3 lambert(Hit hit){
    return hit.mat.color / PI;
}

vec3 oren_nayar(Hit hit, vec3 normal, vec3 lightDir, vec3 viewDir, float roughness){
    vec3 lambert = lambert(hit);
    if (roughness < 1e-2) return lambert;

    float NdotL = max(dot(normal, lightDir), 0);
    float NdotV = max(dot(normal, viewDir), 0);
    
    float A = 1 / (1 + CONSTANT1_FON * roughness);
    float B = roughness * A;

    // FON
    float cosPhi = max(0, cosPhiDiff(normal, lightDir, viewDir));
    float s2 = cosPhi * cosPhi * (1 - NdotL * NdotL) * (1 - NdotV * NdotV);
    float tq = cosPhi <= 0 ? 1 : max(NdotL, NdotV);
    vec3 angleTerm = vec3(A + B * sqrt(s2) / tq);

    // EON normalization
    float EL = E_FON_approx(NdotL, roughness);
    float EV = E_FON_approx(NdotV, roughness);
    float avgE = A * (1 + CONSTANT2_FON * roughness);
    vec3 F = hit.mat.color * avgE / (vec3(1) - hit.mat.color * (1 - avgE));
    angleTerm += F * max(1 - EL, EPS) * max(1 - EV, EPS) / max(1 - avgE, EPS);

    return lambert * angleTerm;
}

void diffuse(inout RaycastData data){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);
    ray.origin += hit.t * ray.dir + EPS * hit.normal;
    
    // MIS
    Primitive light;
    if (numLights > 0)
        light = primitives[lightIndicies[int(rand(seed) * numLights)]];
    float orenNayarRoughness = diffuseRoughness(hit.mat);   
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
        float pbsdf = p_cosineHemisphere(hit.normal, lightDir);
        float weight = computeWeight(pdirect, pbsdf);

        vec3 f_r = oren_nayar(hit, hit.normal, lightDir, viewDir, orenNayarRoughness);
        float NdotL = max(dot(hit.normal, lightDir), 0);
        vec3 Le = light.mat.color * emitIntensity(light.mat);

        ray.radiance += clamp(ray.throughput * f_r * weight * NdotL / pdirect, 0.0, 1.3) * Le;
    }

    // BSDF sampling
    vec3 bsdfDir = randomCosineHemisphere(seed, hit.normal, 1);
    float pbsdf = p_cosineHemisphere(hit.normal, bsdfDir);
    float NdotL = max(0, dot(bsdfDir, hit.normal));
    vec3 f_r = oren_nayar(hit, hit.normal, bsdfDir, viewDir, orenNayarRoughness);
    ray.throughput *= f_r * NdotL / pbsdf;
    ray.dir = bsdfDir;
    ray.pbsdf = pbsdf;

    updateData(data);
    russianRoulette(data);
}