#define Y_INTEGRAL 106.85
#ifdef SPECTRAL
    #define CLAMP_VAL (Y_INTEGRAL * 1.3 * 300)
#else
    #define CLAMP_VAL 1.3
#endif
#define SQRT_PI_OVER_8 0.62665706

int meshBvhInfosIndex(int index){
    return index;
}

int hairBvhInfosIndex(int index){
    return numMeshes + index;
}

int lightIndex(int index){
    return index;
}

int volumeMeshIndex(int index){
    return numLights + index;
}

int volumePrimitiveIndex(int index){
    return numLights + numVolumesMeshes + index;
}

float gaussian(float std, float x){
    float inside = x / std;
    return inversesqrt(2 * PI * std * std) * exp(- 0.5 * inside * inside);
}

float logitCDF(float x, float s)
{
    return 1.0f / (1.0f + exp(-x / s));
}

float logit(float x, float s, float a, float b){
    s *= SQRT_PI_OVER_8;
    float e = exp(-x / s);
    float Fa = logitCDF(a, s);
    float Fb = logitCDF(b, s);
    return e / (s * (1 + e) * (1 + e) * (Fa - Fb));
}

float sampleLogit(float u, float s, float a, float b)
{
    s *= SQRT_PI_OVER_8;
    float Fa = logitCDF(a, s);
    float Fb = logitCDF(b, s);

    float y = Fa + u * (Fb - Fa);

    return s * log(y / (1.0f - y));
}

mat3 rotationMatrix(vec3 rotationDegrees){
    vec3 r = radians(rotationDegrees);
    float cx = cos(r.x), sx = sin(r.x);
    float cy = cos(r.y), sy = sin(r.y);
    float cz = cos(r.z), sz = sin(r.z);

    mat3 rx = transpose(mat3(
        1,  0,   0,
        0,  cx, -sx,
        0,  sx,  cx
    ));

    mat3 ry = transpose(mat3(
        cy,  0, sy,
        0,   1, 0,
        -sy, 0, cy
    ));

    mat3 rz = transpose(mat3(
        cz, -sz, 0,
        sz,  cz, 0,
        0,   0,  1
    ));

    return rz * ry * rx;
}

float dot2(vec2 x){
    return dot(x, x);
}
float dot2(vec3 x){
    return dot(x, x);
}
float dot2(vec4 x){
    return dot(x, x);
}

vec2 ratio(vec2 vec){
    return vec2(vec.x * texSize.x / texSize.y, vec.y);
}

vec3 reflect(vec3 I, vec3 N) {
    return I - 2.0 * dot(I, N) * N;
}

bool isEqual(Primitive prim1, Primitive prim2){
    return prim1.type == prim2.type
    && prim1.pos == prim2.pos
    && prim1.scale == prim2.scale
    && prim1.matIndex == prim2.matIndex;
}

Ray makeRay(vec3 origin, vec3 dir){
    return Ray(
        origin, 
        dir, 
        Spectrum(1),
        Spectrum(0),
        -1
#ifdef SPECTRAL
        ,Spectrum(0)
#endif
    );
}

vec3 refract(vec3 I, vec3 N, float n) {
    vec3 r_perp = n * (I - N * dot(N, I));
    float k = 1 - dot(r_perp, r_perp);
    if (k < 0.0) return vec3(0.0);
    vec3 r_para = - sqrt(k) * N;
    return normalize(r_perp + r_para);
}

float luminanceMean(vec3 c){
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

float luminanceMean(vec4 c){
    return dot(c, vec4(0.25));
}

void stop(inout Hit hit, bool touchedLight){
    hit.t = touchedLight ? -2 : -1;
}

float linearToDepth(float depth, float near, float far){
    float temp = (far + near) / (far - near) - (2.0 * far * near) / (depth * (far - near));
    return (temp + 1) / 2;
}

float sigmoid(float x){
    return 0.5 + 0.5 * x / sqrt(1 + x * x);
}

float sampleSpectrum(float lambda, vec3 c){
    float x = lambda * lambda * c.x + lambda * c.y + c.z;
    return sigmoid(x);
}

vec4 sampleSpectrum(vec4 lambda, vec3 c){
    vec4 result;
    result.x = sampleSpectrum(lambda.x, c);
    result.y = sampleSpectrum(lambda.y, c);
    result.z = sampleSpectrum(lambda.z, c);
    result.w = sampleSpectrum(lambda.w, c);
    return result;
}

float sampleVisibleWavelengths(float u){
    return 538.0 - 138.888889 * atanh(0.85691062 - 1.82750197 * u);
}

float visibleWavelengthsPDF(float lambda){
    if (lambda < 360.0 || lambda > 830.0) return 0.0;
    float x = 0.0072 * (lambda - 538.0);
    float sech = 1.0 / cosh(x);
    return 0.0039398042 * sech * sech;
}

// Approximation analytique des CIE 1931 color matching functions

float gaussian(float x, float alpha, float mu, float sigma1, float sigma2){
    float sigma = (x < mu) ? sigma1 : sigma2;
    float t = (x - mu) / sigma;
    return alpha * exp(-0.5 * t * t);
}

vec3 wavelengthToXYZ(float lambda){
    float x = gaussian(lambda, 1.056, 599.8, 37.9, 31.0)
            + gaussian(lambda, 0.362, 442.0, 16.0, 26.7)
            + gaussian(lambda, -0.065, 501.1, 20.4, 26.2);

    float y = gaussian(lambda, 0.821, 568.8, 46.9, 40.5)
            + gaussian(lambda, 0.286, 530.9, 16.3, 31.1);

    float z = gaussian(lambda, 1.217, 437.0, 11.8, 36.0)
            + gaussian(lambda, 0.681, 459.0, 26.0, 13.8);

    return vec3(x, y, z) / Y_INTEGRAL;
}

vec3 XYZToLinearSRGB(vec3 xyz){
    mat3 adaptE_to_D65 = transpose(mat3(
        0.95315, -0.02655,  0.02385,
        -0.03821,  1.02890,  0.00940,
        0.00260, -0.00303,  1.08937
    ));
    mat3 M = transpose(mat3(
         3.240479, -1.537150, -0.498535,
        -0.969256,  1.875991,  0.041556,
         0.055648, -0.204043,  1.057311
    ));
    return M * adaptE_to_D65 * xyz;
}

vec3 wavelengthToRGB(float lambda){
    vec3 xyz = wavelengthToXYZ(lambda);
    vec3 rgb = XYZToLinearSRGB(xyz);
    return max(rgb, vec3(0.0));
}