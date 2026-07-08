#define SCATTERED -10

void volume(inout RaycastData data, inout Mat volumeMat){ // TODO : hit from non volume to another non volume inside a volume
    Ray ray; Hit hit; uint seed;
    unwrapData(data);

    float sigma_s = scatteringFactor(volumeMat);
    float sigma_a = absorptionFactor(volumeMat);
    if (sigma_s > EPS){
        float scatterDistance = -log(1 - rand(seed) * (1 - EPS)) / sigma_s;
        if (scatterDistance < hit.t - EPS){
            ray.origin += ray.dir * scatterDistance;
            ray.dir = randomOnUnitSphere(seed);
            vec3 absorption = exp(-(vec3(1) - volumeMat.color) * sigma_a * scatterDistance);
            ray.throughput *= absorption;
            volumeMat.data = vec4(SCATTERED);
            updateData(data);
            russianRoulette(data);
            return;
        }
    }

    vec3 absorption = exp(-(vec3(1) - volumeMat.color) * sigma_a * hit.t);
    ray.throughput *= absorption;

    updateData(data);
}