#include <stdexcept>

namespace cista {

inline void cista_verify(bool const condition, char const* msg) {
  if (!condition) {
    throw std::runtime_error(msg);
  }
}

}  // namespace cista