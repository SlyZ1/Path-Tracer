#include "opengl_cuda.hpp"

cudaGraphicsResource* OpenGlCuda::textureToCuda(GLuint texId, GLenum target, unsigned int flags){
    cudaGraphicsResource* resource = nullptr;
    cudaGraphicsGLRegisterImage(&resource, texId, target, flags);
    return resource;
}


cudaGraphicsResource* OpenGlCuda::bufferToCuda(GLuint buffId, unsigned int flags){
    cudaGraphicsResource* resource = nullptr;
    cudaGraphicsGLRegisterBuffer(&resource, buffId, flags);
    return resource;
}

void OpenGlCuda::unmapResources(cudaGraphicsResource** resources, int count){
    if (count < 1 || resources == nullptr) return;
    cudaGraphicsUnmapResources(count, resources);
}

void OpenGlCuda::transferProperty(cudaGraphicsResource** resources, int count){
    if (count < 1 || resources == nullptr) return;
    cudaGraphicsMapResources(count, resources);
}