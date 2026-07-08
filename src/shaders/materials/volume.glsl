#define SCATTERED -10

void volume(inout RaycastData data, inout Hit previousHit){ // TODO : hit from non volume to another non volume inside a volume
    Ray ray; Hit hit; uint seed;
    unwrapData(data);
    //if (dot(ray.dir, previousHit.normal) >= 0.0) return;
    Hit volumeHit; volumeHit.t = -1;
    if (hit.mat.type == MAT_GLASS && hit.inside) volumeHit = hit;
    else if (previousHit.mat.type == MAT_GLASS && dot(ray.dir, previousHit.normal) < 0) volumeHit = previousHit;

    if (volumeHit.t < 0) return;

    float sigma_s = scatteringFactor(volumeHit.mat);
    float sigma_a = absorptionFactor(volumeHit.mat);
    if (sigma_s > EPS){
        float scatterDistance = -log(1 - rand(seed) * (1 - EPS)) / sigma_s;
        if (scatterDistance < hit.t - EPS){
            ray.origin += ray.dir * scatterDistance;
            ray.dir = randomOnUnitSphere(seed);
            vec3 absorption = exp(-(vec3(1) - volumeHit.mat.color) * sigma_a * scatterDistance);
            ray.throughput *= absorption;
            previousHit.t = SCATTERED;
            updateData(data);
            russianRoulette(data);
            return;
        }
    }

    vec3 absorption = exp(-(vec3(1) - volumeHit.mat.color) * sigma_a * hit.t);
    ray.throughput *= absorption;

    updateData(data);
}