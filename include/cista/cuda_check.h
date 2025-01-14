#pragma once

#if defined(__CUDA_ARCH__)
#define CISTA_CUDA_COMPAT __host__ __device__
#else
#define CISTA_CUDA_COMPAT
#endif