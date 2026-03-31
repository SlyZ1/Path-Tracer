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
    return float(seed) * (1.0 / 4294967296.0);
}

vec3 randomInSphere(inout uint seed) {
    float u = rand(seed);
    float v = rand(seed);
    float w = rand(seed);

    float theta = 2.0 * PI * u;
    float phi   = acos(2.0 * v - 1.0);
    float r     = pow(w, 1.0/3.0);

    float x = r * sin(phi) * cos(theta);
    float y = r * sin(phi) * sin(theta);
    float z = r * cos(phi);

    return vec3(x, y, z);
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