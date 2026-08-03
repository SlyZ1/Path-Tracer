#include "material.hpp"


Material Material::diffuseMaterial(vec3 color, float roughness){
    Material mat;
    mat.type = MatType::DIFFUSE;
    mat.color = color;
    mat.color2 = vec3(0.0f);
    mat.data = vec4(glm::clamp(roughness, 0.0f, 1.0f), 0.0f, 0.0f, 0.0f);
    mat.data2 = vec4(0.0f);
    return mat;
}

Material Material::metalMaterial(vec3 color, float fuzziness, float filmIOR, float filmDepth){
    Material mat;
    mat.type = MatType::METAL;
    mat.color = color;
    mat.color2 = color;
    mat.data = vec4(glm::clamp(fuzziness, 0.0f, 1.0f), 0.0f, 0.0f, 0.0f);
    mat.data2 = vec4(0.0f, 0.0f, filmIOR, filmDepth);
    return mat;
} 

Material Material::glassMaterial(
    vec3 color, 
    float fuzziness, 
    float refractionIndex, 
    float dispertionFactor, 
    float absorptionFactor, 
    float scatteringFactor,
    float anisotropy)
{
    Material mat;
    mat.type = MatType::GLASS;
    mat.color = color;
    mat.color2 = vec3(1.0f);
    mat.data = vec4(fuzziness, glm::max(refractionIndex, 0.0f), absorptionFactor, scatteringFactor);
    mat.data2 = vec4(dispertionFactor, anisotropy, 0.0f, 0.0f);
    return mat;
}

Material Material::glossyMaterial(vec3 color, float fuzziness, float metallic){
    Material mat;
    mat.type = MatType::GLOSSY;
    mat.color = color;
    mat.color2 = vec3(1.0f);
    mat.data = vec4(glm::clamp(fuzziness, 0.0f, 1.0f), glm::clamp(metallic, 0.0f, 1.0f), 0.0f, 0.0f);
    mat.data2 = vec4(0.0f, 0.0f, 1.0f, 0.0f);
    return mat;
}

Material Material::furMaterial(vec3 color, float roughA, float roughL, float alpha){
    Material mat;
    mat.type = MatType::FUR;
    mat.color = color;
    mat.color2 = vec3(1.0f);
    mat.data = vec4(glm::clamp(roughA, 0.0f, 90.0f),glm::clamp(roughL, 0.0f, 90.0f),glm::clamp(alpha, 0.0f, 5.0f),0);
    mat.data2 = vec4(0.0f, 0.0f, 1.0f, 0.0f);
    return mat;
}

Material Material::emitMaterial(vec3 color, float intensity){
    Material mat;
    mat.type = MatType::EMIT;
    mat.color = color;
    mat.color2 = vec3(1.0f);
    mat.data = vec4(glm::max(intensity, 0.0f), 0.0f, 0.0f, 0.0f);
    mat.data2 = vec4(0.0f);
    return mat;
}

template <typename Predicate>
int FindInterval(int sz, const Predicate &pred) {
    int size = sz - 2, first = 1;
    while (size > 0) {
        int half = size >> 1, middle = first + half;
        bool predResult = pred(middle);
        first = predResult ? middle + 1 : first;
        size = predResult ? size - (half + 1) : half;
    }
    return glm::clamp(first - 1, 0, sz - 2);
}

vec3 Material::rgbToSigmoidCoeffs(vec3 rgb){
    if (rgb.x == rgb.y && rgb.y == rgb.z){
        float v = clamp(rgb.x, 1e-5f, 1.0f - 1e-5f);
        return vec3(0.0f, 0.0f, (v - 0.5f) / sqrt(v * (1.0f - v)));
    }

    int maxc = (rgb.x > rgb.y) ? ((rgb.x > rgb.z) ? 0 : 2) : ((rgb.y > rgb.z) ? 1 : 2);
    float z = rgb[maxc];
    float x = rgb[(maxc + 1) % 3] * (pbrt::sRGBToSpectrumTable_Res - 1) / z;
    float y = rgb[(maxc + 2) % 3] * (pbrt::sRGBToSpectrumTable_Res - 1) / z;

    const double* zNodes = pbrt::sRGBToSpectrumTable_Scale;
    int yi = std::min((int)y, pbrt::sRGBToSpectrumTable_Res - 2);
    int xi = std::min((int)x, pbrt::sRGBToSpectrumTable_Res - 2); 
    int zi = FindInterval(pbrt::sRGBToSpectrumTable_Res, [&](int i) { return (float)zNodes[i] < z; });
    float dx = x - xi; 
    float dy = y - yi;
    float dz = (z - (float)zNodes[zi]) / ((float)zNodes[zi + 1] - (float)zNodes[zi]);

    vec3 c = vec3(0.0f);
    for (int i = 0; i < 3; ++i) {
        auto co = [&](int dx, int dy, int dz) {
            return (float)pbrt::sRGBToSpectrumTable_Data[maxc][zi + dz][yi + dy][xi + dx][i];
        };
        c[i] =  mix(mix(mix(co(0, 0, 0), co(1, 0, 0), dx),
                        mix(co(0, 1, 0), co(1, 1, 0), dx), dy),
                    mix(mix(co(0, 0, 1), co(1, 0, 1), dx),
                        mix(co(0, 1, 1), co(1, 1, 1), dx), dy), dz);
    }
    return c;
}