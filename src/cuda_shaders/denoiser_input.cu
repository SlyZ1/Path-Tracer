__global__ void packDenoiserInputKernel(
    float* input,
    cudaTextureObject_t albedoTex,
    cudaTextureObject_t colorTex,
    cudaTextureObject_t normalTex,
    int W, int H
){
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= W || y >= H) return;

    int idx = y * W + x;
    float4 alb = tex2D<float4>(albedoTex, (float)x, (float)y);
    float4 col = tex2D<float4>(colorTex, (float)x, (float)y);
    col = make_float4(fmaxf(col.x, 0.0f), fmaxf(col.y, 0.0f), fmaxf(col.z, 0.0f), 1.0f);
    col = make_float4(log1p(col.x), log1p(col.y), log1p(col.z), 1.0f);
    float4 norm = tex2D<float4>(normalTex, (float)x, (float)y);

    idx = (H - 1 - y) * W + x;
    int HW = H * W;
    input[0 * HW + idx] = alb.x;
    input[1 * HW + idx] = alb.y;
    input[2 * HW + idx] = alb.z;
    input[3 * HW + idx] = col.x;
    input[4 * HW + idx] = col.y;
    input[5 * HW + idx] = col.z;
    input[6 * HW + idx] = norm.x;
    input[7 * HW + idx] = norm.y;
    input[8 * HW + idx] = norm.z;
}

void packDenoiserInput(
    dim3 grid, dim3 block, 
    cudaStream_t stream,
    float* input, 
    cudaTextureObject_t albedoTex, 
    cudaTextureObject_t colorTex, 
    cudaTextureObject_t normalTex,
    int W, int H
) {
    packDenoiserInputKernel<<<grid, block, 0, stream>>>(input, albedoTex, colorTex, normalTex, W, H);
}