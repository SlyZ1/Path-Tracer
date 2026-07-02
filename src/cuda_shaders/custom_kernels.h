#pragma once
#include <cuda_runtime.h>

void packDenoiserInput(
    dim3 grid, dim3 block,  
    cudaStream_t stream,
    float* input, 
    cudaTextureObject_t albedoTex, 
    cudaTextureObject_t colorTex, 
    cudaTextureObject_t normalTex,
    int W, int H
);

void unpackDenoiserOutput(
    dim3 grid, dim3 block,  
    cudaStream_t stream,
    cudaSurfaceObject_t outputSurf,
    float* output,
    int W, int H
);