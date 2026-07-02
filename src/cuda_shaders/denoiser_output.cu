__global__ void unpackDenoiserOutputKernel(
    cudaSurfaceObject_t outputSurf,
    float* output,
    int W, int H
){
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= W || y >= H) return;

    int idx = y * W + x;
    int HW = H * W;
    float4 outputValue = make_float4(output[0 * HW + idx], output[1 * HW + idx], output[2 * HW + idx], 1.0f);

    surf2Dwrite(outputValue, outputSurf, x * sizeof(float4), y);
}

void unpackDenoiserOutput(
    dim3 grid, dim3 block, 
    cudaStream_t stream,
    cudaSurfaceObject_t outputSurf, 
    float* output,
    int W, int H
) {
    unpackDenoiserOutputKernel<<<grid, block, 0, stream>>>(outputSurf, output, W, H);
}