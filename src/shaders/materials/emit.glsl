void emit(inout RaycastData data, bool inVolume){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);

    float weight = 1;
    if (!inVolume && ray.pbsdf > 0){
        float LdotNl = max(0, dot(-ray.dir, hit.normal));
        float pdirect = p_direct(primitives[hit.index], hit.t, LdotNl);
        weight = computeWeight(ray.pbsdf, pdirect);
    }

    Spectrum Le = getSpectrumValue(hit.mat) * emitIntensity(hit.mat);
    ray.radiance += clamp(ray.S0 * weight, 0.0, CLAMP_VAL) * Le;
    stop(hit, true);

    updateData(data);
}