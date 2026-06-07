#define LAMBERT 0
#define QON 1
#define FON 2
#define EON 3

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
    if (bsdfType == LAMBERT || roughness < 1e-2) return lambert;

    float NdotL = max(dot(normal, lightDir), 0);
    float NdotV = max(dot(normal, viewDir), 0);

    if (bsdfType == QON){
        float sigma2 = pow(roughness * PI * 0.5, 2);
        float A = 1 - 0.5 * sigma2 / (sigma2 + 0.33);
        float B = 0.45 * sigma2 / (sigma2 + 0.09);

        float cosPhi = max(0, cosPhiDiff(normal, lightDir, viewDir));
        float s2 = cosPhi * cosPhi * (1 - NdotL * NdotL) * (1 - NdotV * NdotV);
        float oneOverTq = cosPhi <= 0 ? 0 : 1 / max(NdotL, NdotV);
        float angleTerm = A + B * sqrt(s2) * oneOverTq;
        return angleTerm * lambert;
    }
    else {
        float A = 1 / (1 + CONSTANT1_FON * roughness);
        float B = roughness * A;

        float cosPhi = max(0, cosPhiDiff(normal, lightDir, viewDir));
        float s2 = cosPhi * cosPhi * (1 - NdotL * NdotL) * (1 - NdotV * NdotV);
        float tq = cosPhi <= 0 ? 1 : max(NdotL, NdotV);
        vec3 angleTerm = vec3(A + B * sqrt(s2) / tq);

        if (bsdfType == EON){
            float EL = E_FON_approx(NdotL, roughness);
            float EV = E_FON_approx(NdotV, roughness);
            float avgE = A * (1 + CONSTANT2_FON * roughness);
            vec3 F = hit.mat.color * avgE / (vec3(1) - hit.mat.color * (1 - avgE));
            angleTerm += F * max(1 - EL, EPS) * max(1 - EV, EPS) / max(1 - avgE, EPS);
        }
        return lambert * angleTerm;
    }

    return lambert;
}

void diffuse(inout RaycastData data){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);
    ray.origin += hit.t * ray.dir + EPS * hit.normal;
    
    // MIS
    float wdirect = 0;
    Primitive light;
    if (numLights > 0)
        light = primitives[lightIndicies[int(rand(seed) * numLights)]];
        wdirect = 0.2;
    float wbsdf = 1 - wdirect;
    float r = rand(seed);
    float orenNayarRoughness = diffuseRoughness(hit.mat);   
    vec3 viewDir = -ray.dir;

    if (r <= wbsdf){
        // BSDF sampling
        vec3 newDir = randomCosineHemisphere(seed, hit.normal, 1);
        vec3 f_r = oren_nayar(hit, hit.normal, newDir, viewDir, orenNayarRoughness);
        ray.throughput *= f_r * PI / wbsdf;
        ray.dir = newDir;

        // Check if we hit a light with the BSDF sampling
        Hit nextHit = rayIntersection(ray);
        if (nextHit.t > 0 && nextHit.mat.type == MAT_EMIT){
            vec3 Le = nextHit.mat.color * emitIntensity(nextHit.mat);
            float LdotNl = max(dot(-newDir, nextHit.normal), 1);

            float pdirect = p_direct(light, nextHit.t, LdotNl)
                            * shadow_hit(light, ray);
            float pbsdf = p_cosineHemisphere(hit.normal, newDir);
            float weight = wbsdf * pbsdf / (wbsdf * pbsdf + wdirect * pdirect);

            ray.radiance += clamp(ray.throughput * weight, 0.0, 1.3) * Le;
            stop(hit, true);
        }
    } 
    else {
        // Direct lighting
        updateData(data);
        float pdirect = sampleLight(data, light);
        unwrapData(data);
        vec3 f_r = oren_nayar(hit, hit.normal, ray.dir, viewDir, orenNayarRoughness);
        ray.throughput *= f_r;

        if (shadow_hit(light, ray) > 0){
            float pbsdf = p_cosineHemisphere(hit.normal, ray.dir);
            float weight = 1.0 / (wdirect * pdirect + wbsdf * pbsdf);
            vec3 Le = light.mat.color * emitIntensity(light.mat);
            ray.radiance += clamp(ray.throughput * weight, 0.0, 1.3) * Le;
            stop(hit, true);
        }
        else{
            ray.throughput /= pdirect * wdirect;
            stop(hit, false);
        }
    }

    updateData(data);
    russianRoulette(data);
}