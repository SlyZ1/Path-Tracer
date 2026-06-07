void glass(inout RaycastData data){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);

    float n = 1 / glassIndex(hit.mat);
    vec3 normal = hit.normal;
    if (hit.inside){
        n = 1 / n;
        normal *= -1;
    }

    vec3 refractedRay = refract(ray.dir, normal, n);
    float cos1 = dot(normal, -ray.dir);
    float cos2 = dot(-normal, refractedRay);
    bool totalReflection = n * n * (1 - cos1 * cos1) > 1;

    float reflectance = fresnel(cos1, cos2, n);
    if (totalReflection || rand(seed) < reflectance){
        data.hit.mat.color = vec3(1);
        ray.origin += hit.t * ray.dir + EPS * normal;
        ray.dir = reflect(ray.dir, hit.normal);
    }
    else{
        ray.origin += hit.t * ray.dir - EPS * normal;
        ray.dir = refractedRay;
    }

    if (hit.inside){
        vec3 absorption = exp(-(vec3(1) - hit.mat.color) * hit.t); // Beer-Lambert
        ray.throughput *= absorption;
    }

    updateData(data);
}