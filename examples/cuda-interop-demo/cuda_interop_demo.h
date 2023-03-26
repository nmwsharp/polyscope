#pragma once

#include <tuple>
#include <vector>
#include <array>

#include <cuda.h>
#include <cuda_runtime_api.h>
#include <curand.h>
#include <curand_kernel.h>

void printCUDAInfo();
std::tuple<uint32_t,uint32_t> computeBlockThreadSizes(size_t nPts);

void initializeCUDAData(float3*& positionBuffer, curandState*& randState, size_t nPts);
void freeCUDAData(float3*& positionBuffer, curandState*& randState);

void initializeOpenGLMappedBuffer(uint32_t glTargetBuffID, cudaGraphicsResource*& cudaGraphicsResource);
void freeOpenGLMappedBuffer(cudaGraphicsResource*& cudaGraphicsResource);

void updateCUDAData();
void diffusePositions(float3* positionBuffer, curandState* randState, float stepSize, size_t nPts);
std::vector<std::array<float,3>> getPositionsCPU(float3*& positionBuffer, size_t nPts);

void copyPositionsToGL(float3*& positionBuffer, cudaGraphicsResource*& cudaGraphicsResource, size_t nPts);
