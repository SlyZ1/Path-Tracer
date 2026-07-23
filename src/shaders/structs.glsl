struct Camera {
    vec3 pos;
    vec3 lookDir;
};

struct Mat {
    vec3 color;
    int pad;
    vec3 color2;
    int type;
    vec4 data;
    vec4 data2;
};

struct Primitive {
    vec3 pos;
    int matIndex;
    vec3 scale;
    int type;
    vec3 rotation;
    int pad;
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
    int leaf;
    int pad;
    AABB aabb;
    AABB leftAabb;
    AABB rightAabb;
};

struct BVHInfos {
    vec3 pos;
    int leafOffset;
    vec3 scale;
    int nodeOffset;
    vec3 rotation;
    int numberOfNodes;
    vec3 data;
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
    #define getSpectrumValueFromColor(c) sampleSpectrum(ray.lambda, c)
    #define WHITE_COLOR vec3(0.0, 0.0, 0.5 / sqrt(0.99 * 0.01))
#else
    #define getSpectrumValue(m) m.color
    #define getSpectrumValueFromColor(c) c
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
    vec3 normal;
    float t;
    vec3 tangent;
    int primIndex;
    Mat mat;
    bool inside;
};

struct RaycastData {
    Hit hit;
    Ray ray;
    uint seed;
};

#ifdef SPECTRAL

struct FresnelConductorParams {
    Mat mat;
    Spectrum lambda;
    Spectrum spectrumValue;
    Spectrum spectrumValue2;
};
#define newFresnelParams(v, v2) FresnelConductorParams(hit.mat, ray.lambda, v, v2)

struct FresnelDielectricParams {
    Mat mat;
    Spectrum lambda;
    SpectralParam n;
};
#define newFresnelDielectricParams(n) FresnelDielectricParams(hit.mat, ray.lambda, n)

#else

struct FresnelConductorParams {
    Spectrum spectrumValue;
    Spectrum spectrumValue2;
};
#define newFresnelParams(v, v2) FresnelConductorParams(v, v2)

struct FresnelDielectricParams {
    SpectralParam n;
};
#define newFresnelDielectricParams(n) FresnelDielectricParams(n)

#endif

struct PointAndDir {
    vec3 point;
    vec3 dir;
};