void emit(inout RaycastData data){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);

    float weight = 1;
    if (ray.pbsdf > 0){
        float LdotNl = max(0, dot(-ray.dir, hit.normal));
        float pdirect = p_direct(primitives[lightIndicies[0]], hit.t, LdotNl);
        weight = computeWeight(ray.pbsdf, pdirect);
    }
    vec3 Le = hit.mat.color * emitIntensity(hit.mat);
    ray.radiance += clamp(data.ray.throughput * weight, 0.0, 1.3) * Le;
    stop(hit, true);

    updateData(data);
}