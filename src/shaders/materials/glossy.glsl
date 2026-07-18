void glossy(inout RaycastData data){
    Ray ray; Hit hit; uint seed;
    unwrapData(data);
    ray.pbsdf = -1;

    vec3 normal = hit.normal;
    float metallic = pbrMetallic(hit.mat);
    float n = 1 - metallic;

    vec3 refractedRay = refract(ray.dir, normal, n);
    float cos1 = dot(normal, -ray.dir);
    float cos2 = dot(-normal, refractedRay);
    bool totalReflection = n * n * (1 - cos1 * cos1) > 1;

    float reflectance = fresnel(cos1, cos2, n);
    if (totalReflection || rand(seed) <= reflectance){
        data.hit.mat.data = vec4(pbrFuzz(hit.mat), 0.0, 0.0, 0.0);
        data.hit.mat.data2 = vec4(0.0, 0.0, 1.0, 0.0);
        data.hit.mat.color = WHITE_COLOR;
        data.hit.mat.color2 = WHITE_COLOR;
        metal(data);
    }
    else{
        data.hit.mat.data = vec4(0.0);
        data.hit.mat.data2 = vec4(0.0);
        diffuse(data);
    }
}