#include <stdexcept>

#ifndef cista_verify
#define cista_verify(A, M)       \
  if (!(A)) {                    \
    throw std::runtime_error(M); \
  }
#endif