void glossy(World world, inout RaycastData data){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);

    vec3 normal = hit.normal;
    float metallic = pbrMetallic(hit.mat);
    float n = 1 - metallic;

    vec3 refractedRay = refract(ray.dir, normal, n);
    float cos1 = dot(normal, -ray.dir);
    float cos2 = dot(-normal, refractedRay);
    bool totalReflection = n * n * (1 - cos1 * cos1) > 1;

    float reflectance = fresnel(cos1, cos2, n);
    if (totalReflection || rand(seed) < reflectance){
        data.hit.mat.color = vec3(1);
        metal(world, data);
    }
    else{
        data.hit.mat.data = mData(0, 0);
        diffuse(world, data);
    }
}