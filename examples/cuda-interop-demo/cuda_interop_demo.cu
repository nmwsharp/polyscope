#include "cuda_interop_demo.h"

#include <cuda_gl_interop.h>

#include <iostream>

#define checkCudaErrors(val) check((val), #val, __FILE__, __LINE__)

#ifdef __DRIVER_TYPES_H__
static const char *_cudaGetErrorEnum(cudaError_t error) {
  return cudaGetErrorName(error);
}
#endif

#ifdef CUDA_DRIVER_API
// CUDA Driver API errors
static const char *_cudaGetErrorEnum(CUresult error) {
  static char unknown[] = "<unknown>";
  const char *ret = NULL;
  cuGetErrorName(error, &ret);
  return ret ? ret : unknown;
}
#endif

template <typename T>
void check(T result, char const *const func, const char *const file,
           int const line) {
  if (result) {
    fprintf(stderr, "CUDA error at %s:%d code=%d(%s) \"%s\" \n", file, line,
            static_cast<unsigned int>(result), _cudaGetErrorEnum(result), func);
    exit(EXIT_FAILURE);
  }
}

void printCUDAInfo()
{
    std::cout << "CUDA Info:\n";
    std::cout << "  CUDA Compiler version " << 
                 " Major: " << __CUDACC_VER_MAJOR__ <<
                 " Minor: " << __CUDACC_VER_MINOR__ <<
                 " Build: " << __CUDACC_VER_BUILD__ <<
                 std::endl;

    int runtime_ver;
    cudaRuntimeGetVersion(&runtime_ver);
    std::cout << "  CUDA Runtime version: " << runtime_ver << std::endl;

    int driver_ver;
    cudaDriverGetVersion(&driver_ver);
    std::cout << "  CUDA Driver version: " << driver_ver << std::endl;
}

uint32_t ceilingDivide(uint32_t a, uint32_t b) {
  return a/b + (a % b != 0);
}

std::tuple<uint32_t,uint32_t> computeBlockThreadSizes(size_t nPts) {
  uint32_t nThreads = 256;
  uint32_t nBlocks = ceilingDivide(nPts,nThreads);
  return std::tuple<uint32_t,uint32_t>(nBlocks, nThreads);
}

__global__ void initRNG_kernel(curandState* randState, uint32_t nPts) {
  uint32_t idx = threadIdx.x+blockDim.x*blockIdx.x;
  if (idx >= nPts) return;

  curand_init(1234, idx, 0, &randState[idx]);
}


void initializeCUDAData(float3*& positionBuffer, curandState*& randState, size_t nPts) {

  // Allocate the position buffer
  std::cout << "Allocating CUDA buffer for " << nPts << " pts...\n";
  size_t nBytes = nPts*3*sizeof(float);
  checkCudaErrors(cudaMalloc((void **)&positionBuffer, nBytes));
  checkCudaErrors(cudaMemset(positionBuffer, 0, nBytes));
  std::cout << " ...done.\n";

  uint32_t nBlocks, nThreads;
  std::tie(nBlocks, nThreads) = computeBlockThreadSizes(nPts);
  std::cout << "nBlocks = " << nBlocks << "  nThreads = " << nThreads << std::endl;


  // Allocate & initialize the RNGs
  std::cout << "Allocating RNGs...\n";
  checkCudaErrors(cudaMalloc(&randState, nPts*sizeof(curandState)));
  
  std::cout << "Initializing RNGs...\n";
  
  initRNG_kernel<<<nBlocks,nThreads>>>(randState, nPts);

  checkCudaErrors(cudaPeekAtLastError());
  checkCudaErrors(cudaDeviceSynchronize());

  std::cout << " ...done.\n";
}

void initializeOpenGLMappedBuffer(uint32_t glTargetBuffID, cudaGraphicsResource*& glResource) {
    checkCudaErrors(cudaGraphicsGLRegisterBuffer(&glResource, glTargetBuffID, cudaGraphicsRegisterFlagsWriteDiscard));
}

void freeCUDAData(float3*& positionBuffer, curandState*& randState) {
  cudaFree(positionBuffer);
  cudaFree(randState);
}

void freeOpenGLMappedBuffer(cudaGraphicsResource*& glResource) {
    checkCudaErrors(cudaGraphicsUnregisterResource(glResource));
    glResource = nullptr;
}

__global__ void diffusePositions_kernel(float3* pos, curandState* randState, float stepSize, uint32_t nPts) {
    uint32_t i = threadIdx.x+blockDim.x*blockIdx.x;
    if (i >= nPts) return;

    // get the random state
    curandState localState = randState[i];

    float rx = curand_normal(&localState);
    float ry = curand_normal(&localState);
    float rz = curand_normal(&localState);

    // Update the position
    float3 p = pos[i];
    pos[i] = make_float3(p.x + rx*stepSize, p.y + ry*stepSize, p.z + rz*stepSize);
    
    // store the random state
    randState[i] = localState;
}

void diffusePositions(float3* positionBuffer, curandState* randState, float stepSize, size_t nPts) {
  
  //std::cout << "Diffusing positions...\n";
  
  uint32_t nBlocks, nThreads;
  std::tie(nBlocks, nThreads) = computeBlockThreadSizes(nPts);

  diffusePositions_kernel<<<nBlocks,nThreads>>>(positionBuffer, randState, stepSize, nPts);

  checkCudaErrors(cudaPeekAtLastError());
  checkCudaErrors(cudaDeviceSynchronize());

  //std::cout << " ...done.\n";
}

std::vector<std::array<float,3>> getPositionsCPU(float3*& positionBuffer, size_t nPts) {
  //std::cout << "Copying position state...\n";

  cudaDeviceSynchronize();

  std::vector<std::array<float,3>> positionBuffer_CPU(nPts);
  checkCudaErrors(cudaMemcpy(&positionBuffer_CPU[0][0], positionBuffer, nPts*3*sizeof(float), cudaMemcpyDeviceToHost));

  //std::cout << " ...done.\n";
  return positionBuffer_CPU;
}

__global__ void copyFloat3_kernel(float3* source, float3* target, uint32_t nPts) {
    uint32_t i = threadIdx.x+blockDim.x*blockIdx.x;
    if (i >= nPts) return;

    target[i] = source[i];
}

void copyPositionsToGL(float3*& positionBuffer, cudaGraphicsResource*& glResource, size_t nPts) {

    // map the OpenGL buffer to a CUDA memory block
    checkCudaErrors(cudaGraphicsMapResources(1, &glResource));

    float3* mappedBuff;
    size_t mappedBuffSize; // should == nPts
    checkCudaErrors(cudaGraphicsResourceGetMappedPointer((void**)&mappedBuff, &mappedBuffSize,
                                                         glResource));

    // run a simpel kernel which just copies
    uint32_t nBlocks, nThreads;
    std::tie(nBlocks, nThreads) = computeBlockThreadSizes(nPts);
    copyFloat3_kernel<<<nBlocks,nThreads>>>(positionBuffer, mappedBuff, nPts);

    // unmap the buffer now that we are done with it
    checkCudaErrors(cudaGraphicsUnmapResources(1, &glResource));
}
