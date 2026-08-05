#define SCATTERED -10

void volume(inout RaycastData data, inout Mat volumeMat){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);

    ray.pbsdf = -1;

    Spectrum spectrumValue = getSpectrumValue(volumeMat);

    float sigma_s = scatteringFactor(volumeMat);
    float sigma_a = absorptionFactor(volumeMat);
    float g = anisotropy(volumeMat);
    if (sigma_s > EPS){
        float scatterDistance = -log(1 - rand(seed) * (1 - EPS)) / sigma_s;
        if (scatterDistance < hit.t - EPS){
            ray.origin += ray.dir * scatterDistance;
            ray.dir = sampleHG(seed, ray.dir, g);
            Spectrum absorption = exp(-(Spectrum(1) - spectrumValue) * sigma_a * scatterDistance);
            ray.S0 *= absorption;
            volumeMat.data = vec4(SCATTERED);
            updateData(data);
            russianRoulette(data);
            return;
        }
    }

    Spectrum absorption = exp(-(Spectrum(1) - spectrumValue) * sigma_a * hit.t);
    ray.S0 *= absorption;

    updateData(data);
}