void emit(inout RaycastData data){
    data.ray.radiance += data.ray.throughput * data.hit.mat.color * emitIntensity(data.hit.mat);
    data.hit.t = -1;
}