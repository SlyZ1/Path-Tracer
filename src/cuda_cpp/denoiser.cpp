#include "denoiser.hpp"

void Denoiser::load(const string& path){
    cout << "Loading denoiser engine... (at " << path << ")" << endl;
    m_runtime = createInferRuntime(m_logger);

    ifstream file(path, ios::binary);
    if (!file.is_open()) {
        cerr << "Erreur: impossible d'ouvrir le fichier " << path << endl;
        return;
    }

    std::vector<char> engineData(
       (istreambuf_iterator<char>(file)),
        istreambuf_iterator<char>()
    );

    m_engine = m_runtime->deserializeCudaEngine(engineData.data(), engineData.size());

    cout << "Engine loaded." << endl;
}

void Denoiser::init(GLuint albedoTexture, GLuint colorTexture, GLuint normalTexture, GLuint outputTexture){
    albedoRes = OpenGlCuda::textureToCuda(albedoTexture, GL_TEXTURE_2D, cudaGraphicsRegisterFlagsReadOnly);
    colorRes = OpenGlCuda::textureToCuda(colorTexture, GL_TEXTURE_2D, cudaGraphicsRegisterFlagsReadOnly);
    normalRes = OpenGlCuda::textureToCuda(normalTexture, GL_TEXTURE_2D, cudaGraphicsRegisterFlagsReadOnly);
    outputRes = OpenGlCuda::textureToCuda(outputTexture, GL_TEXTURE_2D, cudaGraphicsRegisterFlagsWriteDiscard);

    cudaError_t err;
    err = cudaMalloc((void**)&input, m_numInputChannels * m_H * m_W * sizeof(float));
    if (err != cudaSuccess)
        std::cerr << "cudaMalloc input failed: " << cudaGetErrorString(err) << std::endl;

    err = cudaMalloc((void**)&output, 3 * m_H * m_W * sizeof(float));
    if (err != cudaSuccess)
        std::cerr << "cudaMalloc output failed: " << cudaGetErrorString(err) << std::endl;

    int device = -1;
    cudaGetDevice(&device);
    std::cerr << "Current CUDA device: " << device << std::endl;
}

void Denoiser::infer(){
    if (m_context == nullptr)
        m_context = m_engine->createExecutionContext();

    if (m_stream == nullptr)
        cudaStreamCreate(&m_stream);

    // Give property from OpenGL to CUDA
    cudaGraphicsResource* resources[] = { albedoRes, colorRes, normalRes, outputRes };
    cudaGraphicsMapResources(4, resources, m_stream);

    // Convert To Optimized Array (in mem)
    cudaArray_t albedoArr, colorArr, normalArr, outputArr;
    cudaGraphicsSubResourceGetMappedArray(&albedoArr, albedoRes, 0, 0);
    cudaGraphicsSubResourceGetMappedArray(&colorArr,  colorRes,  0, 0);
    cudaGraphicsSubResourceGetMappedArray(&normalArr, normalRes, 0, 0);
    cudaGraphicsSubResourceGetMappedArray(&outputArr, outputRes, 0, 0);

    // Get Easy-to-manipulate Texture (interface, how to "view" the data)
    cudaResourceDesc resDesc = {};
    resDesc.resType = cudaResourceTypeArray;
    cudaTextureDesc texDesc = {};
    texDesc.readMode = cudaReadModeElementType;
    texDesc.addressMode[0] = cudaAddressModeClamp;
    texDesc.addressMode[1] = cudaAddressModeClamp;
    texDesc.filterMode = cudaFilterModePoint;
    texDesc.normalizedCoords = 0;
    cudaTextureObject_t albedoTex, colorTex, normalTex;
    resDesc.res.array.array = albedoArr;
    cudaCreateTextureObject(&albedoTex, &resDesc, &texDesc, nullptr);
    resDesc.res.array.array = colorArr;
    cudaCreateTextureObject(&colorTex,  &resDesc, &texDesc, nullptr);
    resDesc.res.array.array = normalArr;
    cudaCreateTextureObject(&normalTex, &resDesc, &texDesc, nullptr);

    // Another view, for writing purposes
    cudaSurfaceObject_t outputSurf;
    resDesc.res.array.array = outputArr;
    cudaCreateSurfaceObject(&outputSurf, &resDesc);

    // Reorganize inputs
    dim3 block(16, 16);
    dim3 grid((m_W + 15) / 16, (m_H + 15) / 16);
    packDenoiserInput(grid, block, m_stream, input, albedoTex, colorTex, normalTex, m_W, m_H);

    // Infer
    m_context->setTensorAddress("input",  input);
    m_context->setTensorAddress("output", output);
    m_context->enqueueV3(m_stream);

    // unpack
    unpackDenoiserOutput(grid, block, m_stream, outputSurf, output, m_W, m_H);

    // sync
    cudaStreamSynchronize(m_stream);

    // Cleaning
    cudaDestroyTextureObject(colorTex);
    cudaDestroyTextureObject(albedoTex);
    cudaDestroyTextureObject(normalTex);
    cudaDestroySurfaceObject(outputSurf);
    cudaGraphicsUnmapResources(4, resources, m_stream);
}

void Denoiser::testInfer() {
    if (m_context == nullptr)
        m_context = m_engine->createExecutionContext();
    if (m_stream == nullptr)
        cudaStreamCreate(&m_stream);

    // Input rempli de 0.5 (valeur safe, pas de NaN/Inf)
    int inputSize  = m_numInputChannels * m_H * m_W;
    int outputSize = 3 * m_H * m_W;

    float* d_test_in  = nullptr;
    float* d_test_out = nullptr;
    cudaMalloc(&d_test_in,  inputSize  * sizeof(float));
    cudaMalloc(&d_test_out, outputSize * sizeof(float));
    
    // Remplir avec 0.5
    std::vector<float> h_in(inputSize, 0.5f);
    cudaMemcpy(d_test_in, h_in.data(), inputSize * sizeof(float), cudaMemcpyHostToDevice);

    m_context->setTensorAddress("input",  d_test_in);
    m_context->setTensorAddress("output", d_test_out);
    bool ok = m_context->enqueueV3(m_stream);
    cudaStreamSynchronize(m_stream);

    std::cerr << "testInfer ok: " << ok << std::endl;
    cudaError_t err = cudaGetLastError();
    std::cerr << "testInfer error: " << cudaGetErrorString(err) << std::endl;

    // Lire quelques valeurs
    std::vector<float> h_out(outputSize);
    cudaMemcpy(h_out.data(), d_test_out, outputSize * sizeof(float), cudaMemcpyDeviceToHost);
    std::cerr << "out[0]=" << h_out[0] << " out[1]=" << h_out[1] << " out[2]=" << h_out[2] << std::endl;

    cudaFree(d_test_in);
    cudaFree(d_test_out);
}