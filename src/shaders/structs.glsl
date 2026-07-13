struct Camera {
    vec3 pos;
    vec3 lookDir;
};

struct Mat {
    vec3 color;
    int type;
    vec4 data;
    vec4 data2;
};

struct Primitive {
    vec3 pos;
    float scale;
    vec2 pad;
    int matIndex;
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
    vec3 pad;
    int matIndex;
};

#ifdef SPECTRAL
    #define Spectrum vec4
    #define SpectralParam vec4
    #define paramToFloat(s) s.x
#else
    #define Spectrum vec3
    #define SpectralParam float
    #define paramToFloat(s) s
#endif

#ifdef SPECTRAL
    #define getSpectrumValue(m) sampleSpectrum(ray.lambda, m.color)
    #define WHITE_COLOR vec3(0.0, 0.0, 0.5 / sqrt(0.99 * 0.01))
#else
    #define getSpectrumValue(m) m.color
    #define WHITE_COLOR vec3(1.0)
#endif

struct Ray {
    vec3 origin;
    vec3 dir;
    Spectrum throughput;
    Spectrum radiance;
    float pbsdf;
#ifdef SPECTRAL
    Spectrum lambda;
#endif
};

struct Hit {
    float t;
    vec3 normal;
    Mat mat;
    bool inside;
    int primIndex;
};

struct RaycastData {
    Hit hit;
    Ray ray;
    uint seed;
};

#ifdef SPECTRAL
struct FresnelParams {
    Mat mat;
    Spectrum lambda;
    Spectrum spectrumValue;
};
#define newFresnelParams(v) FresnelParams(hit.mat, ray.lambda, v)
#else

struct FresnelParams {
    Spectrum spectrumValue;
};
#define newFresnelParams(v) FresnelParams(v)
#endif