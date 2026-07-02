#ifndef MODELLOADER
#define MODELLOADER

#include <iostream>
#include <vector>
#include <fstream>
#include "NvInfer.h"
#include "opengl_cuda.hpp"
#include "../cuda_shaders/custom_kernels.h"

using namespace nvinfer1;
using namespace std;

class Logger : public ILogger {
    void log(Severity severity, const char* msg) noexcept override {
        if (severity <= Severity::kERROR)
            std::cout << msg << std::endl;
    }
};

class Denoiser {
public:
    Denoiser(int H, int W, int numInputChannels) : m_H(H), m_W(W), m_numInputChannels(numInputChannels) {};
    void load(const string& path);
    void init(GLuint albedoTexture, GLuint colorTexture, GLuint normalTexture, GLuint outputTexture);
    void infer();
    void testInfer();
private:
    int m_H = 0;
    int m_W = 0;
    int m_numInputChannels = 0;

    Logger m_logger;
    IRuntime* m_runtime = nullptr;
    ICudaEngine* m_engine = nullptr;
    IExecutionContext* m_context = nullptr;
    cudaStream_t m_stream = nullptr;

    cudaGraphicsResource* albedoRes = nullptr;
    cudaGraphicsResource* colorRes = nullptr;
    cudaGraphicsResource* normalRes = nullptr;
    cudaGraphicsResource* outputRes = nullptr;

    float* input = nullptr;
    float* output = nullptr;
};

#endif