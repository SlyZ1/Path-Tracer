#version 430 core

struct Camera {
    vec3 pos;
    vec3 lookDir;
};

struct Mat {
    vec3 color;
    int type;
    vec4 data;
};

struct Primitive {
    vec3 pos;
    float scale;
    Mat mat;
    vec3 pad;
    int type;
};

struct Triangle {
    vec3 v0;
    vec3 v1;
    vec3 v2;
    vec3 n0;
    vec3 n1;
    vec3 n2;
};

struct AABB {
    vec4 min;
    vec4 max;
};

struct BVHNode {
    int left;
    int right;
    int triangle;
    int pad;
    AABB aabb;
    AABB leftAabb;
    AABB rightAabb;
};

struct MeshInfos {
    vec3 pos;
    float scale;
    int triangleOffset;
    int nodeOffset;
    int numberOfNodes;
    int isSmooth; 
    Mat mat;
};

struct Ray {
    vec3 origin;
    vec3 dir;
    vec3 throughput;
    vec3 radiance;
    float pbsdf;
    float lambda;
};

struct Hit {
    float t;
    vec3 normal;
    Mat mat;
    bool inside;
};


layout(std430, binding = 0) buffer TrianglesBuffer {
    Triangle triangles[];
};
uniform int numTriangles;
layout(std430, binding = 1) buffer BVHBuffer {
    BVHNode nodes[];
};
uniform int numBVHNodes;
uniform int debugBVH;
layout(std430, binding = 4) buffer MeshBuffer {
    MeshInfos meshInfos[];
};
uniform int numMeshes;

layout(std430, binding = 2) buffer PrimitiveBuffer {
    Primitive primitives[];
};
uniform int numPrimitives;
layout(std430, binding = 3) buffer LightIndicesBuffer {
    int lightIndicies[];
};
uniform int numLights;

struct RaycastData {
    Hit hit;
    Ray ray;
    uint seed;
};

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
uniform int frameCount;
uniform int samples;
uniform vec2 winSize;
uniform int maxBounces;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 NormalOut;
layout (location = 2) out vec4 AlbedoOut;
layout (location = 3) out vec4 ColorOut;
layout (location = 4) out vec4 DepthOut;
in vec4 vClipPos;

//#define SAMPLES 1
#define EPS 1e-4
#define PROBA_EPS 1e-6
#define MAX_NONEMIT_BOUNCE 10
#define PI 3.14159265

#define updateData(data) data = RaycastData(hit, ray, seed)
#define unwrapData(data) ray = data.ray; hit = data.hit; seed = data.seed

#define diffuseRoughness(m) m.data.x
#define pbrFuzz(m) m.data.x
#define pbrMetallic(m) m.data.y
#define emitIntensity(m) m.data.x          
#define glassIndex(m) m.data.y
#define absorptionFactor(m) m.data.z
#define scatteringFactor(m) m.data.w

#define MAT_DIFF 0
#define MAT_METAL 1
#define MAT_GLASS 2
#define MAT_GLOSSY 3
#define MAT_EMIT 4

// -------------------- UTILS

#pragma include "./rand.glsl"
#pragma include "./utils.glsl"
#pragma include "./intersections.glsl"

#pragma FDECLARE
// RAND.GLSL
uint initSeed(uvec2 pos, uint frame);
float rand(inout uint seed);
vec3 randomInSphere(inout uint seed);
vec2 randomInDisk(inout uint seed);
vec3 randomOnUnitSphere(inout uint seed);
vec3 randomOnUnitHemiphere(inout uint seed, vec3 normal);
vec3 randomCosineHemisphere(inout uint seed, vec3 normal, float randomizationFactor);
vec3 randomGGXHemisphere(inout uint seed, vec3 normal, float alpha);

// UTILS.GLSL
vec2 ratio(vec2 vec);
vec3 reflect(vec3 I, vec3 N);
vec3 refract(vec3 I, vec3 N, float n);
void stop(inout Hit hit, bool touchedLight);
float luminanceMean(vec3 c);
float linearToDepth(float depth, float near, float far);

// INTERSECTIONS.GLSL
Hit rayIntersection(inout Ray ray, bool isShadow);
#pragma FEND

vec3 schlickFresnel(float VdotN, vec3 F0)
{
    float minusDot = clamp(1 - VdotN, 0, 1);
    float dot2 = minusDot * minusDot;
    float dot5 = dot2 * dot2 * minusDot;
    return F0 + (vec3(1) - F0) * dot5;
}

float fresnel(float cos1, float cos2, float n){
    if (n < 2.3 && n > 1.3){ // validity on schlickFresnel
        float f0 = (1 - n)/(1 + n);
        f0 *= f0;
        return schlickFresnel(cos1, vec3(f0)).x;
    }
    float Fp = (n * cos1 - cos2) / (n * cos1 + cos2);
    float Fs = (cos1 - n * cos2) / (cos1 + n * cos2);
    return 0.5 * (Fp * Fp + Fs * Fs);
}

// pdfs

float computeWeight(float p1, float p2){
    float n1 = p1 * p1;
    float n2 = p2 * p2;
    return n1 / (n1 + n2);
}

float p_direct(Primitive light, float distance, float cosLight){
    return distance * distance / (cosLight * 2 * PI * light.scale * light.scale * numLights + 1e-4);
}

float shadow_hit(Primitive light, Ray ray){
    Hit hit = rayIntersection(ray, true);
    vec3 hitPos = ray.origin + ray.dir * hit.t;
    if (hit.t >= 0 && length(hitPos - light.pos) > light.scale + sqrt(EPS)) return 0;
    return 1;
}

// lighting functions

void russianRoulette(inout RaycastData data){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);
    
    float prob = max(max(ray.throughput.r, ray.throughput.g), ray.throughput.b);
    if (rand(seed) > prob) {
        stop(hit, true);
    }  
    else { ray.throughput /= max(min(prob, 1), EPS); }

    updateData(data);
}

vec4 sampleLight(inout RaycastData data, Primitive light){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);

    vec3 nLight = randomOnUnitHemiphere(seed, ray.origin - light.pos);
    vec3 lightPoint = light.pos + nLight * light.scale;
    vec3 lightDir = normalize(lightPoint - ray.origin);
    float LdotNl = max(dot(-lightDir, nLight), 0);
    
    float distance = length(lightPoint - ray.origin);
    float pdirect = p_direct(light, distance, LdotNl);

    updateData(data);
    return vec4(lightDir, pdirect);
}

// -------------------- MATERIALS

#pragma include "./materials/diffuse.glsl"
#pragma include "./materials/metal.glsl"
#pragma include "./materials/glass.glsl"
#pragma include "./materials/glossy.glsl"
#pragma include "./materials/emit.glsl"
#pragma include "./materials/volume.glsl"

#pragma FDECLARE
void diffuse(inout RaycastData data);
void metal(inout RaycastData data);
void glass(inout RaycastData data);
void glossy(inout RaycastData data);
void emit(inout RaycastData data);
void volume(inout RaycastData data, inout Hit previousHit);
#pragma FEND


void computeLighting(inout Hit hit, inout Hit previousHit, inout Ray ray, inout uint seed){
    if (hit.t < 0) return;

    RaycastData data = RaycastData(hit, ray, seed);
    if (previousHit.t >= 0) volume(data, previousHit); // TODO : hit from non volume to another non volume inside a volume
    if (previousHit.t == SCATTERED){
        unwrapData(data);
        return;
    }
    switch (hit.mat.type){
        case MAT_DIFF:
            diffuse(data);
            break;
        case MAT_GLOSSY:
            glossy(data);
            break;
        case MAT_METAL:
            metal(data);
            break;
        case MAT_GLASS:
            glass(data);
            break;
        case MAT_EMIT:
            emit(data);
            break;
    }
    unwrapData(data);
}

// RAY TRACING --------------------

Ray fovRay(vec2 pos, Ray ray, inout uint seed){
    vec3 forward = normalize(camera.lookDir);
    vec3 worldUp = abs(forward.y) < 0.999
                 ? vec3(0,1,0)
                 : vec3(0,0,1);
    vec3 right = normalize(cross(forward, worldUp));
    vec3 up    = cross(right, forward);

    vec2 lensJitter = randomInDisk(seed) * cameraAperture;
    ray.origin += lensJitter.x * right + lensJitter.y * up;

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
    if (a < 0.35){
        sky = mix(sunset, middle, a / 0.5);
    }
    else{
        sky = mix(middle, top, (a - 0.5) / 0.5);
    }

    return sky * skyIntensity;
}

vec3 sky(vec3 lookingAt){
    return horizon(lookingAt);
}

void tracePath(in out uint seed, Ray ray, out vec4 result, out vec4 normal, out vec4 albedo, out vec4 color, out float depth){
    Ray tracedRay = ray;
    bool firstHit = true;
    normal = vec4(0);
    albedo = vec4(0);
    color = vec4(0);
    depth = 1e2;
    Hit previousHit; previousHit.t = -1;
    for (int i = 0; i < maxBounces; i++){

        Hit hit = rayIntersection(tracedRay, false);
        if (hit.t > 0 && firstHit){
            firstHit = false;
            normal = vec4(normalize(hit.normal), 1);
            albedo = vec4(hit.mat.color, 1);
            depth = hit.t;
        }
        computeLighting(hit, previousHit, tracedRay, seed);
        previousHit = hit;

        if (hit.t < 0){
            if (hit.t > -2) tracedRay.radiance += tracedRay.throughput * sky(tracedRay.dir);
            if (firstHit) albedo = vec4(sky(tracedRay.dir), 1);
            result = vec4(tracedRay.radiance, 1);
            color = result / (albedo + vec4(EPS));
            return;
        }// Cas ou on tombe sur une light ou sur rien
    }
}

// vec4 rayColor(in out uint seed, Ray ray){
//     Ray tracedRay = ray;
//     for (int i = 0; i < maxBounces; i++){

//         Hit hit = rayIntersection(tracedRay, false);
//         //if (hit.t > 0) tracedRay.throughput *= exp(-hit.t * 0.015);
//         computeLighting(hit, tracedRay, seed);

//         if (hit.t < 0){
//             if (hit.t > -2) tracedRay.radiance += tracedRay.throughput * sky(tracedRay.dir);
//             return vec4(tracedRay.radiance, 1);
//         }// Cas ou on tombe sur une light ou sur rien
//     }

//     return vec4(0);
// }

void main()
{
    float resolutionFactor = winSize.x / texSize.x;
    uint seed = initSeed(uvec2(gl_FragCoord.xy * resolutionFactor), frameCount);
    vec2 pos = ratio(vClipPos.xy) * resolutionFactor + ratio(vec2(1)) * (resolutionFactor - 1);
    vec2 offset = resolutionFactor * vec2(rand(seed), rand(seed)) / winSize;
    vec2 uv = (vClipPos.xy + vec2(1)) * 0.5;

    Ray ray = Ray(
        camera.pos, 
        camera.lookDir, 
        vec3(1),
        vec3(0),
        -1
    );

    vec4 radiance = vec4(0);
    vec4 normal = vec4(0);
    vec4 albedo = vec4(0);
    vec4 color = vec4(0);
    float depth = 0;
    for (int i = 0; i < samples; i++) {
        vec2 AAjitter = vec2(rand(seed), rand(seed)) * 2 / winSize;
        Ray r = fovRay(pos + AAjitter, ray, seed);

        vec4 result = vec4(0);
        tracePath(seed, r, result, normal, albedo, color, depth);
        radiance += max(result, vec4(0));
    }

    vec4 finalResult = radiance + max(frameCount, 0) * texture(screenTex, uv);
    finalResult /= frameCount + samples;
    FragColor = finalResult;
    NormalOut = normal;
    AlbedoOut = albedo;
    ColorOut = color;
    depth = clamp(depth / 1e2, 0.0, 1.0);
    DepthOut = vec4(depth);
}