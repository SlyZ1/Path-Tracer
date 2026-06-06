#version 430 core

struct Camera {
    vec3 pos;
    vec3 lookDir;
};

struct Mat {
    int type;
    vec3 color;
    float[2] data;
};

struct Sphere {
    vec3 pos;
    float rad;
    Mat mat;
};

struct Triangle {
    vec3 v0;
    vec3 v1;
    vec3 v2;
};

struct AABB {
    vec3 min;
    vec3 max;
};

struct BVHNode {
    AABB aabb;
    int left;
    int right;
    int triangle;
};

struct Light {
    vec3 pos;
    float rad;
    vec3 color;
    float intensity;
};

struct Plane {
    vec3 origin;
    vec3 normal;
    Mat mat;
};

struct Ray {
    vec3 origin;
    vec3 dir;
    vec3 throughput;
    vec3 radiance;
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
uniform vec3 modelPos;

#define NUM_SPHERE 3
#define NUM_PLANE 1
#define NUM_LIGHT 1

struct World {
    Sphere spheres[NUM_SPHERE];
    Plane planes[NUM_PLANE];
#if NUM_LIGHT > 0
    Light lights[NUM_LIGHT];
#endif
};

struct RaycastData {
    Hit hit;
    Ray ray;
    uint seed;
};

out vec4 FragColor;
layout (location = 1) uniform vec4 metalProperties;
layout (location = 2) uniform vec2 texSize;
layout (location = 3) uniform sampler2D screenTex;
layout (location = 4) uniform int frameCount;
layout (location = 5) uniform int bsdfType;
layout (location = 6) uniform float ballRoughness;
layout (location = 7) uniform int samples;
layout (location = 8) uniform vec2 winSize;
layout (location = 9) uniform int maxBounces;
layout (location = 10) uniform float refractionIndex;
layout (location = 11) uniform bool useModel;
uniform Camera camera;
in vec4 vClipPos;

//#define SAMPLES 1
#define EPS 1e-4
#define PROBA_EPS 1e-6
#define MAX_NONEMIT_BOUNCE 10
#define PI 3.14159265
#define CONSTANT1_FON (0.5 - 2 / (3 * PI))
#define CONSTANT2_FON (2 / 3 - 28 / (15 * PI))

#define MAT_DIFF 0
#define MAT_GLOSSY 1
#define MAT_EMIT 2
#define MAT_GLASS 3
#define MAT_METAL 4

#define updateData(data) data = RaycastData(hit, ray, seed)
#define unwrapData(data) ray = data.ray; hit = data.hit; seed = data.seed

#define mData(d0, d1) float[2](d0, d1)
#define mData0(d) float[2](d, 0)
#define mData1(d) float[2](0, d)
#define mNoData() float[2](0, 0)

#define diffuseRoughness(m) m.data[0]
#define pbrFuzz(m) m.data[0]              
#define pbrMetallic(m) m.data[1]
#define emitIntensity(m) m.data[0]          
#define glassIndex(m) m.data[1]

#define BSDF_LAMBERT 0
#define BSDF_LAMBERT_WRAP 1
#define BSDF_OREN_NAYAR 2

#define LAMBERT 0
#define QON 1
#define FON 2
#define EON 3

// -------------------- UTILS

#pragma include "./rand.glsl"
#pragma include "./utils.glsl"
#pragma include "./intersections.glsl"

#pragma FDECLARE
// RAND.GLSL
uint initSeed(uvec2 pos, uint frame);
float rand(inout uint seed);
vec3 randomInSphere(inout uint seed);
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

// INTERSECTIONS.GLSL
Hit rayIntersection(World world, inout Ray ray);
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

float p_direct(Light light, float distance, float cosLight){
    return distance * distance / (cosLight * 2 * PI * light.rad * light.rad * NUM_LIGHT);
}

float shadow_hit(Light light, World world, Ray ray){
    Hit hit = rayIntersection(world, ray);
    vec3 hitPos = ray.origin + ray.dir * hit.t;
    if (hit.t > 0 && length(hitPos - light.pos) > light.rad + sqrt(EPS)) return 0;
    return 1;
}

// lighting functions

void russianRoulette(inout RaycastData data){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);
    
    float prob = max(max(ray.throughput.r, ray.throughput.g), ray.throughput.b);
    if (rand(seed) > prob) {
        stop(hit, false);
    }  
    else { ray.throughput /= max(min(prob, 1), EPS); }

    updateData(data);
}

float sampleLight(inout RaycastData data, Light light){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);

    vec3 nLight = randomOnUnitHemiphere(seed, ray.origin - light.pos);
    vec3 lightPoint = light.pos + nLight * light.rad;
    vec3 newDir = normalize(lightPoint - ray.origin);
    ray.dir = newDir;

    float NdotL = max(dot(hit.normal, newDir), 0);
    float LdotNl = max(dot(-newDir, nLight), 0);
    
    float distance = length(lightPoint - ray.origin);
    float pdirect = p_direct(light, distance, LdotNl);
    ray.throughput *= NdotL;

    updateData(data);
    return pdirect;
}

// -------------------- MATERIALS

#pragma include "./materials/diffuse.glsl"
#pragma include "./materials/metal.glsl"
#pragma include "./materials/glass.glsl"
#pragma include "./materials/glossy.glsl"
#pragma include "./materials/emit.glsl"

#pragma FDECLARE
void diffuse(World world, inout RaycastData data);
void metal(World world, inout RaycastData data);
void glass(World world, inout RaycastData data);
void glossy(World world, inout RaycastData data);
void emit(inout RaycastData data);
#pragma FEND

void computeLighting(World world, in out Hit hit, in out Ray ray, in out uint seed){
    if (hit.t < 0) return;

    RaycastData data = RaycastData(hit, ray, seed);
    switch (hit.mat.type){
        case MAT_DIFF:
            diffuse(world, data);
            break;
        case MAT_GLOSSY:
            glossy(world, data);
            break;
        case MAT_METAL:
            metal(world, data);
            break;
        case MAT_GLASS:
            glass(world, data);
            break;
        case MAT_EMIT:
            emit(data);
            break;
    }
    unwrapData(data);
}

// RAY TRACING --------------------

Ray fovRay(vec2 pos, Ray ray){
    float fov = radians(mix(50.0, 90.0, 0 / 8.0));
    vec3 forward = normalize(camera.lookDir);

    vec3 worldUp = abs(forward.y) < 0.999
                 ? vec3(0,1,0)
                 : vec3(0,0,1);

    vec3 right = normalize(cross(forward, worldUp));
    vec3 up    = cross(right, forward);

    float tanHalfFov = tan(fov * 0.5);

    vec3 dir = forward + (right * pos.x + up * pos.y) * tanHalfFov;
    ray.dir = normalize(dir);
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

vec3 horizon(vec3 lookingAt){
    float a = (dot(normalize(lookingAt), vec3(0,1,0)) + 1) * 0.5;
    vec3 top = vec3(0.32, 0.55, 0.78);
    vec3 bot = vec3(0.62, 0.72, 0.85);
    return mix(bot, top, a) * 0.7;
}

vec3 sky(vec3 lookingAt){
    return horizon(lookingAt);
}

vec4 rayColor(World world, in out uint seed, Ray ray){
    Ray tracedRay = ray;
    for (int i = 0; i < maxBounces; i++){

        Hit hit = rayIntersection(world, tracedRay);
        //if (hit.t > 0) tracedRay.throughput *= exp(-hit.t * 0.015);
        computeLighting(world, hit, tracedRay, seed);

        if (hit.t < 0){
            if (hit.t > -2) tracedRay.radiance += tracedRay.throughput * sky(tracedRay.dir);
            return vec4(tracedRay.radiance, 1);
        }// Cas ou on tombe sur une light ou sur rien
    }

    return vec4(0);
}

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
        vec3(0)
    );
    ray = fovRay(pos, ray);

    Mat planeMat = Mat(MAT_GLOSSY, vec3(0.85), mData(0,0.4));
    Mat sphereMat = Mat(MAT_DIFF, vec3(1, 0, 0), mData0(ballRoughness));
    Mat glassMat = Mat(MAT_GLASS, vec3(1, 0.85, 0.85), mData0(refractionIndex));

    float metallic = metalProperties.w;
    vec3 metalColor = metalProperties.rgb;
    Mat metalMat1 = Mat(MAT_GLOSSY, metalColor, mData(0 / 8.0, metallic));
    Mat metalMat2 = Mat(MAT_GLOSSY, metalColor, mData(2 / 8.0, metallic));
    Mat metalMat3 = Mat(MAT_GLOSSY, metalColor, mData(4 / 8.0, metallic));
    Mat metalMat4 = Mat(MAT_GLOSSY, metalColor, mData(6 / 8.0, metallic));

    Mat sphereMat1 = Mat(MAT_GLASS, metalColor, mData1(1.05));
    Mat sphereMat2 = Mat(MAT_GLASS, metalColor, mData(ballRoughness, refractionIndex));
    Mat sphereMat3 = Mat(MAT_GLASS, metalColor, mData1(4));

    Sphere spheres[NUM_SPHERE];
    spheres[0] = Sphere(vec3(3,3,-4), 1, metalMat1);
    spheres[1] = Sphere(vec3(0,1,-4), 1, sphereMat2);
    /*spheres[2] = Sphere(vec3(-3,1, -4), 1, sphereMat3);
    spheres[3] = Sphere(vec3(10, EPS, -5), 2, metalMat3);
    spheres[4] = Sphere(vec3(16, EPS, -5), 2, metalMat4);
    spheres[5] = Sphere(vec3(-6, EPS, -5), 2, glassMat);*/
    
    Plane planes[NUM_PLANE];
    planes[0] = Plane(vec3(0,0,-5), vec3(0,1,0), planeMat);

#if NUM_LIGHT > 0
    Light lights[NUM_LIGHT];
    lights[0] = Light(modelPos, 1.5, vec3(1), 10);
    //lights[1] = Light(vec3(6,5,10), 1.5, vec3(1), 10);
    World world = World(spheres, planes, lights);
#else
    World world = World(spheres, planes);
#endif

    vec4 radiance;
    for(int i = 0; i < samples; i++){
        radiance += max(rayColor(world, seed, ray), 0);
    }

    FragColor = radiance + max(frameCount, 0) * texture(screenTex, uv);
    FragColor /= frameCount + samples;
}