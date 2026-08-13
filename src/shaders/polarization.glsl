#define depolarizer(x) mat4(x,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0)

vec3 computeDefaultDirBasis(vec3 dir){
    return normalize(vec3(0,1,0) - dir * dir.y);
}

mat4 muellerRotator(float cosTheta, float sinTheta){
    float s = 2 * cosTheta * sinTheta;      // sin(2 theta)
    float c = 2 * cosTheta * cosTheta - 1;  // cos(2 theta)
    return transpose(mat4(
        1, 0, 0, 0,
        0, c, s, 0,
        0, -s, c, 0,
        0, 0, 0, 0
    ));
}

vec3 transportBasis(vec3 currentBasis, vec3 currentPropagAxis, vec3 targetPropagAxis){
    vec3 crossR = cross(currentPropagAxis, targetPropagAxis);
    float sinTheta = length(crossR);
    float cosTheta = dot(currentPropagAxis, targetPropagAxis);
    
    if (sinTheta < 1e-6){
        return (cosTheta > 0.0) ? currentBasis : -currentBasis;
    }
    
    vec3 k = crossR / sinTheta;
    return currentBasis * cosTheta + cross(k, currentBasis) * sinTheta + k * dot(k, currentBasis) * (1.0 - cosTheta);
}

vec3 computeTargetBasis(vec3 currentBasis, vec3 normalAxis, vec3 propagAxis){
    vec3 targetBasis;
    float VdotN = dot(propagAxis, normalAxis);
    if (abs(VdotN) < 0.999){
        targetBasis = normalize(normalAxis - propagAxis * VdotN);
    }
    else{
        targetBasis = currentBasis;
    }
    return targetBasis;
}

mat4 rotateBasis(vec3 currentBasis, vec3 normalAxis, vec3 propagAxis){
    vec3 targetBasis = computeTargetBasis(currentBasis, normalAxis, propagAxis);
    float cosTheta = dot(currentBasis, targetBasis);
    float sinTheta = dot(targetBasis, cross(propagAxis, currentBasis));
    return muellerRotator(cosTheta, sinTheta);
}

mat4 rotateBasis(vec3 currentBasis, vec3 currentPropagAxis, vec3 targetBasis, vec3 targetPropagAxis){
    currentBasis = transportBasis(currentBasis, currentPropagAxis, targetPropagAxis);
    float cosTheta = dot(currentBasis, targetBasis);
    float sinTheta = dot(targetBasis, cross(targetPropagAxis, currentBasis));
    return muellerRotator(cosTheta, sinTheta);
}

Spectrum getMuellerCoeff(in Ray ray, int i, int j){
    Spectrum result;
    for(int k = 0; k < SPECTRUM_DIM; k++) {
        result[k] = ray.mueller[k][i][j];
    }
    return result;
}

void applyMuellerMatrix(inout Ray ray, in mat4[SPECTRUM_DIM] mueller){
    for(int i = 0; i < SPECTRUM_DIM; i++) {
        ray.mueller[i] = ray.mueller[i] * mueller[i];
    }
}

void applyMuellerMatrix(inout Ray ray, in mat4 mueller){
    for(int i = 0; i < SPECTRUM_DIM; i++) {
        ray.mueller[i] = ray.mueller[i] * mueller;
    }
}

void applyDepolarizer(inout Ray ray, in Spectrum spectrum){
    for(int i = 0; i < SPECTRUM_DIM; i++) {
        ray.mueller[i] = ray.mueller[i] * depolarizer(spectrum[i]);
    }
}

void applyDepolarizer(inout Ray ray, float spectrum){
    for(int i = 0; i < SPECTRUM_DIM; i++) {
        ray.mueller[i] = ray.mueller[i] * depolarizer(spectrum);
    }
}

void applyMultiplier(inout Ray ray, in Spectrum spectrum){
    for(int i = 0; i < SPECTRUM_DIM; i++) {
        ray.mueller[i] *= spectrum[i];
    }
}

void applyMultiplier(inout Ray ray, float spectrum){
    for(int i = 0; i < SPECTRUM_DIM; i++) {
        ray.mueller[i] *= spectrum;
    }
}