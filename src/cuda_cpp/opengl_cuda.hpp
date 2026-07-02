#ifndef OPENGLCUDA
#define OPENGLCUDA

#include <glad/glad.h>
#include <cuda_gl_interop.h>

class OpenGlCuda {
public:
    OpenGlCuda() {};
    static cudaGraphicsResource* textureToCuda(GLuint texId, GLenum target, unsigned int flags);
    static cudaGraphicsResource* bufferToCuda(GLuint buffId, unsigned int flags);
    static void unmapResources(cudaGraphicsResource** resources, int count);
    static void transferProperty(cudaGraphicsResource** resources, int count);
};

#endif