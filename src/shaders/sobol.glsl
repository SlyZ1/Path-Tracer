uint laineKarrasPermutation(uint x, uint seed){
    x += seed;
    x ^= x * 0x6c50b47cu;
    x ^= x * 0xb82f1e52u;
    x ^= x * 0xc7afe638u;
    x ^= x * 0x8d22f6e6u;
    return x;
}

uint nestedUniformScramble(uint x, uint seed){
    x = bitfieldReverse(x);
    x = laineKarrasPermutation(x, seed);
    x = bitfieldReverse(x);
    return x;
}

uint hashCombine(uint seed, uint i){
    return seed ^ (i * 0x9e3779b9u + (seed << 6) + (seed >> 2));
}

// --- Direction numbers Sobol (à valider contre une référence, cf. discussion précédente) ---
uint sobolDirections[4][32];

void initSobolDirections(){
    for (int i = 0; i < 32; i++){
        sobolDirections[0][i] = 1u << (31 - i);
    }
    {
        uint m1 = 1u;
        sobolDirections[1][0] = m1 << 31;
        for (int i = 1; i < 32; i++){
            sobolDirections[1][i] = sobolDirections[1][i-1] ^ (sobolDirections[1][i-1] >> 1);
        }
    }
    {
        uint v[32];
        v[0] = 1u << 31;
        v[1] = 3u << 30;
        for (int i = 2; i < 32; i++){
            v[i] = v[i-2] ^ (v[i-2] >> 2) ^ (v[i-1] >> 1) ^ (v[i-1]);
        }
        for (int i = 0; i < 32; i++) sobolDirections[2][i] = v[i];
    }
    {
        uint v[32];
        v[0] = 1u << 31;
        v[1] = 3u << 30;
        v[2] = 7u << 29;
        for (int i = 3; i < 32; i++){
            v[i] = v[i-3] ^ (v[i-3] >> 3) ^ (v[i-1] >> 1);
        }
        for (int i = 0; i < 32; i++) sobolDirections[3][i] = v[i];
    }
}

uint sobol1d(uint index, int dim){
    uint result = 0u;
    uint i = index;
    int bit = 0;
    while (i != 0u){
        if ((i & 1u) != 0u){
            result ^= sobolDirections[dim][bit];
        }
        i >>= 1u;
        bit++;
    }
    return result;
}

// --- Version vectorisée : uvec4 au lieu de uint[4] ---
uvec4 sobol4d(uint index){
    return uvec4(
        sobol1d(index, 0),
        sobol1d(index, 1),
        sobol1d(index, 2),
        sobol1d(index, 3)
    );
}

uvec4 nestedUniformScrambleVec(uvec4 x, uint seed){
    return uvec4(
        nestedUniformScramble(x.x, hashCombine(seed, 0u)),
        nestedUniformScramble(x.y, hashCombine(seed, 1u)),
        nestedUniformScramble(x.z, hashCombine(seed, 2u)),
        nestedUniformScramble(x.w, hashCombine(seed, 3u))
    );
}

uvec4 shuffledScrambledSobol4d(uint index, uint seed){
    index = nestedUniformScramble(index, seed);
    uvec4 X = sobol4d(index);
    return nestedUniformScrambleVec(X, seed);
}

// Conversion finale en float [0,1)
vec4 sampleSobol4d(uint sampleIndex, uint seed){
    uvec4 X = shuffledScrambledSobol4d(sampleIndex, pixelSeed);
    seed = hashCombine(seed, 0x9e3779b9u);
    return vec4(X) * (1.0 / 4294967296.0);
}

uint initSeed(ivec2 coord, uint frameCount){
    return hashCombine(uint(pixelCoord.x) ^ (uint(pixelCoord.y) << 16u), frameNumber);
}