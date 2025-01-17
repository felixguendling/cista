#pragma once

#if defined(__CUDA_ARCH__)
#define CISTA_CUDA_COMPAT __host__ __device__
#define CISTA_CUDA_DEVICE_COMPAT __device__
#else
#define CISTA_CUDA_COMPAT
#define CISTA_CUDA_DEVICE_COMPAT
#endif