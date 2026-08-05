uint pcg_hash(uint v) {
    v = v * 747796405u + 2891336453u;
    uint word = ((v >> ((v >> 28u) + 4u)) ^ v) * 277803737u;
    return (word >> 22u) ^ word;
}

uint initSeed(uvec2 pos, uint frame) {
    uint v = pos.x + pos.y * 4096u + frame * 1315423911u;
    return pcg_hash(v);
}

float rand(inout uint seed) {
    seed = pcg_hash(seed);
    float val = float(seed) * (1.0 / 4294967296.0);
    return clamp(val, 0.0, 1.0);
}

vec3 randomInSphere(inout uint seed) {
    float u = rand(seed);
    float v = rand(seed);
    float w = rand(seed);

    float theta = 2.0 * PI * u;
    float phi = acos(2.0 * v - 1.0);
    float r = pow(w, 1.0/3.0);

    float x = r * sin(phi) * cos(theta);
    float y = r * sin(phi) * sin(theta);
    float z = r * cos(phi);

    return vec3(x, y, z);
}

vec2 randomInDisk(inout uint seed) {
    float u = rand(seed);
    float v = rand(seed);

    float theta = 2.0 * PI * u;
    float r = pow(v, 1.0/2.0);

    float x = r * cos(theta);
    float y = r * sin(theta);

    return vec2(x, y);
}

vec2 randomOnUnitCircle(inout uint seed){
    float u = rand(seed);
    float theta = 2.0 * PI * u;
    float x = cos(theta);
    float y = sin(theta);
    return vec2(x, y);
}

vec3 randomOnUnitSphere(inout uint seed) {
    float u = rand(seed);
    float v = rand(seed);

    float theta = 2.0 * PI * u;
    float phi   = acos(2.0 * v - 1.0);

    float x = sin(phi) * cos(theta);
    float y = sin(phi) * sin(theta);
    float z = cos(phi);

    return vec3(x, y, z);
}

vec3 randomOnUnitHemiphere(inout uint seed, vec3 normal){
    vec3 dir = randomOnUnitSphere(seed);
    if (dot(dir, normal) < 0)
        dir *= -1;
    return dir;
}

vec3 randomCosineHemisphere(inout uint seed, vec3 normal, float randomizationFactor){
    vec3 val = normalize(normal) * (1 + EPS) + randomOnUnitSphere(seed) * randomizationFactor;
    return normalize(val);
}

void createTangentBasis(in vec3 N, out vec3 T, out vec3 B) {
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);

    T = normalize(cross(up, N));
    B = cross(N, T);
}

vec3 sampleHG(inout uint seed, vec3 dir, float g){
    float s = rand(seed) * 2.0 - 1.0;
    float cosTheta;
    if (abs(g) < 1e-3){
        cosTheta = s;
    }
    else{
        float sqrTerm = (1 - g*g) / (1 + g * s);
        cosTheta = (1 + g*g - sqrTerm * sqrTerm) / (2 * g);
    }
    float sinTheta = sqrt(1 - cosTheta * cosTheta);

    vec2 circle = randomOnUnitCircle(seed);

    vec3 up = abs(dir.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
    vec3 right = normalize(cross(dir, up));
    up = normalize(cross(right, dir));

    return cosTheta * dir + sinTheta * (circle.x * right + circle.y * up);
}

vec3 randomGGXHemisphere(inout uint seed, vec3 normal, float alpha){
    float xi1 = rand(seed);
    float xi2 = rand(seed);
    float phi = 2.0 * PI * xi2;
    float theta = atan(alpha * sqrt(xi1) / sqrt(1.0 - xi1));
    vec3 H_local = vec3(
        sin(theta) * cos(phi), 
        sin(theta) * sin(phi), 
        cos(theta)
    );

    vec3 T, B;
    createTangentBasis(normal, T, B);
    vec3 H = normalize(H_local.x * T + H_local.y * B + H_local.z * normal);

    return H;
}

vec3 randomGGX_VNDFHemisphere(inout uint seed, vec3 wo, vec3 wn, float alpha){
    if (alpha < EPS) return wn;
    
    vec3 T, B;
    createTangentBasis(wn, T, B);
    vec3 wo_local = vec3(dot(wo, T), dot(wo, B), dot(wo, wn));
    
    vec3 wo_t = normalize(vec3(alpha * wo_local.x, alpha * wo_local.y, wo_local.z));
    
    float lengthXY = length(vec2(wo_t.x, wo_t.y));
    vec3 T1 = lengthXY > EPS ? vec3(-wo_t.y, wo_t.x, 0.0) / lengthXY : vec3(1.0, 0.0, 0.0);
    vec3 T2 = cross(wo_t, T1);

    float x1 = rand(seed);
    float x2 = rand(seed);
    float r = sqrt(x1);
    float phi = 2.0 * PI * x2;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5 * (1.0 + wo_t.z);
    t2 = (1.0 - s) * sqrt(1.0 - t1 * t1) + s * t2;

    vec3 wm_t = t1 * T1 + t2 * T2 + sqrt(max(0.0, 1.0 - t1*t1 - t2*t2)) * wo_t;
    vec3 wm_local = normalize(vec3(alpha * wm_t.x, alpha * wm_t.y, max(0.0, wm_t.z)));
    
    vec3 wm = normalize(wm_local.x * T + wm_local.y * B + wm_local.z * wn);
    return wm;
}

#ifdef SPECTRAL
void sampleLambda(inout Ray ray, inout uint seed){
    ray.lambda.x = mix(380.0, 780.0, rand(seed));
    ray.lambda.y = mix(380.0, 780.0, rand(seed));
    ray.lambda.z = mix(380.0, 780.0, rand(seed));
    ray.lambda.w = mix(380.0, 780.0, rand(seed));
    ray.S0 *= 780.0 - 380.0;   
}
#endif