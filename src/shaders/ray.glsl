#version 430 core

#pragma include "structs.glsl"

layout(std430, binding = 0) buffer TrianglesBuffer {
    Triangle triangles[];
};

layout(std430, binding = 6) buffer HairStrandsBuffer {
    vec4 hairPoints[];
};

layout(std430, binding = 1) buffer BVHBuffer {
    BVHNode nodes[];
};
uniform int numBVHNodes;
uniform int debugBVH;

layout(std430, binding = 2) buffer PrimitiveBuffer {
    Primitive primitives[];
};
uniform int numPrimitives;

layout(std430, binding = 3) buffer IndicesBuffer {
    int indicies[];
};
uniform int numLights;
uniform int numVolumesMeshes;
uniform int numVolumesPrims;

layout(std430, binding = 4) buffer BVHInfosBuffer {
    BVHInfos bvhInfos[];
};
uniform int numMeshes;
uniform int numHair;

layout(std430, binding = 5) buffer MaterialBuffer {
    Mat matBuffer[];
};

uniform int selectedObject;

uniform Camera camera;
uniform float cameraFov;
uniform float cameraAperture;
uniform float cameraFocalLength;

uniform float skyIntensity;
uniform vec3 skyBottomColor;
uniform vec3 skyMiddleColor;
uniform vec3 skyTopColor;

uniform vec2 texSize;
uniform sampler2D screenTex;
uniform sampler2D envMap;
uniform int frameCount;
uniform int samples;
uniform vec2 winSize;
uniform int maxBounces;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 ResultColor;
layout (location = 2) out vec4 NormalOut;
layout (location = 3) out vec4 AlbedoOut;
layout (location = 4) out vec4 ColorOut;
layout (location = 5) out vec4 DepthOut;
layout (location = 6) out vec4 SelectionOut;
in vec4 vClipPos;

//#define SAMPLES 1
#define EPS 1e-4
#define PROBA_EPS 1e-6
#define MAX_NONEMIT_BOUNCE 10
#define PI 3.14159265

#define updateData(data) data = RaycastData(hit, ray, seed)
#define unwrapData(data) ray = data.ray; hit = data.hit; seed = data.seed

#define betaN(m) m.data.x
#define betaM(m) m.data.y
#define alpha(m) m.data.z
#define diffuseRoughness(m) m.data.x
#define pbrFuzz(m) m.data.x
#define pbrMetallic(m) m.data.y
#define emitIntensity(m) m.data.x
#define glassIndex(m) m.data.y
#define absorptionFactor(m) m.data.z
#define scatteringFactor(m) m.data.w
#define dispertionFactor(m) m.data2.x
#define anisotropy(m) m.data2.y
#define filmIOR(m) m.data2.z
#define filmDepth(m) m.data2.w

#define isSmooth(b) b.data.x
#define radiusMultiplier(b) b.data.x

#define hairPointRadius(p) p.w
#define hairPointPos(p) p.xyz

#define MAT_DIFF 0
#define MAT_METAL 1
#define MAT_GLASS 2
#define MAT_GLOSSY 3
#define MAT_FUR 4
#define MAT_EMIT 5

// -------------------- UTILS

#pragma include "./rand.glsl"
#pragma include "./utils.glsl"
#pragma include "./reflections.glsl"
#pragma include "./intersections.glsl"
#pragma include "./mis-nee.glsl"
#pragma include "./polarization.glsl"

// -------------------- MATERIALS

#pragma include "./materials/diffuse.glsl"
#pragma include "./materials/metal.glsl"
#pragma include "./materials/glass.glsl"
#pragma include "./materials/glossy.glsl"
#pragma include "./materials/fur.glsl"
#pragma include "./materials/emit.glsl"
#pragma include "./materials/volume.glsl"

void computeLighting(inout Hit hit, inout Ray ray, inout uint seed){
    if (hit.t < 0) return;

    RaycastData data = RaycastData(hit, ray, seed);
    Mat volumeMat;
    bool inVolume = isInVolume(ray, volumeMat);
    if (inVolume) volume(data, volumeMat);
    if (volumeMat.data == vec4(SCATTERED)){
        unwrapData(data);
        return;
    }
    switch (hit.mat.type){
        case MAT_DIFF:
            diffuse(data, inVolume);
            break;
        case MAT_GLOSSY:
            glossy(data, inVolume);
            break;
        case MAT_METAL:
            metal(data, inVolume);
            break;
        case MAT_GLASS:
            glass(data, inVolume);
            break;
        case MAT_FUR:
            fur(data, inVolume);
            break;
        case MAT_EMIT:
            emit(data, inVolume);
            break;
    }
    unwrapData(data);
}

// RAY TRACING --------------------

Ray fovRay(vec2 pos, Ray ray, inout uint seed, bool lensBlur){
    vec3 forward = normalize(camera.lookDir);
    vec3 worldUp = abs(forward.y) < 0.999
                 ? vec3(0,1,0)
                 : vec3(0,0,1);
    vec3 right = normalize(cross(forward, worldUp));
    vec3 up    = cross(right, forward);

    if (lensBlur){
        vec2 lensJitter = randomInDisk(seed) * cameraAperture;
        ray.origin += lensJitter.x * right + lensJitter.y * up;
    }

    float fov = radians(cameraFov);
    float tanHalfFov = tan(fov * 0.5);

    vec3 focusPoint = camera.pos + (forward + (right * pos.x + up * pos.y) * tanHalfFov) * cameraFocalLength;

    ray.dir = normalize(focusPoint - ray.origin);
    return ray;
}

vec3 sun(vec3 lookingAt)
{
    vec3 sunDir = normalize(vec3(0.2, 0.6, 0.7));
    float sunAngularRadius = radians(4);
    float sunIntensity = 50.0;
    float cosAngle = dot(lookingAt, sunDir);
    float sun = smoothstep(
        cos(sunAngularRadius),
        cos(sunAngularRadius * 0.8),
        cosAngle
    );
    return vec3(sunIntensity) * sun;
}

vec3 horizon(vec3 lookingAt)
{
    float a = (dot(normalize(lookingAt), vec3(0,1,0)) + 1.0) * 0.5;

    vec3 top = skyTopColor;
    vec3 middle = skyMiddleColor;
    vec3 sunset = skyBottomColor;

    vec3 sky;
    if (a < 0.5){
        sky = mix(sunset, middle, a / 0.5);
    }
    else{
        sky = mix(middle, top, (a - 0.5) / 0.5);
    }

    return sky;
}

vec3 sky(vec3 lookingAt){
    return horizon(lookingAt);
}

vec2 dirToEquirectUV(vec3 dir) {
    float phi = atan(dir.z, dir.x);   // longitude, [-π, π]
    float theta = acos(clamp(dir.y, -1.0, 1.0)); // latitude, [0, π] (0 = pôle haut)

    float u = (phi + PI) / (2.0 * PI);
    float v = theta / PI;
    return vec2(u, v);
}

vec3 sampleEnvironment(vec3 dir) {
    vec2 uv = dirToEquirectUV(dir);
    return texture(envMap, uv).rgb;
}

void tracePath(in out uint seed, Ray ray, out vec4 result, out vec4 normal, out vec4 albedo, out vec4 color, out float depth){
    bool firstHit = true;
    normal = vec4(0);
    albedo = vec4(0);
    color = vec4(0);
    depth = 1e2;
    for (int i = 0; i < maxBounces; i++){
        float hitSelected;
        Hit hit = rayIntersection(ray, hitSelected, false);

        if (hit.t > 0 && firstHit){
            firstHit = false;
            normal = vec4(normalize(hit.normal), 1);
            albedo = vec4(hit.mat.color, 1);
            depth = hit.t;
        }

        computeLighting(hit, ray, seed);

        if (hit.t < 0){
#ifdef SPECTRAL
            if (hit.t > -2) ray.radiance += ray.S0 * sampleSpectrum(ray.lambda, sky(ray.dir)) * skyIntensity;

            result = vec4(wavelengthToXYZ(ray.lambda.x) * ray.radiance.x, 1);
            result += vec4(wavelengthToXYZ(ray.lambda.y) * ray.radiance.y, 1);
            result += vec4(wavelengthToXYZ(ray.lambda.z) * ray.radiance.z, 1);
            result += vec4(wavelengthToXYZ(ray.lambda.w) * ray.radiance.w, 1);
            result /= 4.0;
#else
            if (hit.t > -2) ray.radiance += ray.S0 * sky(ray.dir) * skyIntensity;
            if (firstHit) albedo = vec4(sky(ray.dir), 1);
            result = vec4(ray.radiance, 1);
            color = result / (albedo + vec4(EPS));
#endif
            return;
        }
    }
}

void main()
{
    float resolutionFactor = winSize.x / texSize.x;
    uint seed = initSeed(uvec2(gl_FragCoord.xy * resolutionFactor), frameCount);
    vec2 pos = ratio(vClipPos.xy) * resolutionFactor + ratio(vec2(1)) * (resolutionFactor - 1);
    vec2 offset = resolutionFactor * vec2(rand(seed), rand(seed)) / winSize;
    vec2 uv = (vClipPos.xy + vec2(1)) * 0.5;

    Ray ray = makeRay(camera.pos, camera.lookDir);
#ifdef SPECTRAL
    sampleLambda(ray, seed);
#endif

    vec4 radiance = vec4(0);
    vec4 normal = vec4(0);
    vec4 albedo = vec4(0);
    vec4 color = vec4(0);
    float depth = 0.0;
    for (int i = 0; i < samples; i++) {
        vec2 AAjitter = vec2(rand(seed), rand(seed)) * 2 / winSize;
        Ray r = fovRay(pos + AAjitter, ray, seed, true);

        vec4 result = vec4(0);
        tracePath(seed, r, result, normal, albedo, color, depth);
        radiance += max(result, vec4(0));
    }

    vec4 accumulation = radiance + max(frameCount, 0) * texture(screenTex, uv);
    accumulation /= frameCount + samples;
    FragColor = accumulation;
#ifdef SPECTRAL
    ResultColor = vec4(XYZToLinearSRGB(accumulation.xyz), 1.0);
#else
    ResultColor = accumulation;
#endif
    NormalOut = normal;
    AlbedoOut = albedo;
    ColorOut = color;
    depth = clamp(depth / 1e2, 0.0, 1.0);
    DepthOut = vec4(depth);

    float hitSelected = 0.0;
    Ray untransformedRay = fovRay(pos, ray, seed, false);
    Hit hit = rayIntersection(untransformedRay, hitSelected, true);
    SelectionOut = vec4(hitSelected);
}