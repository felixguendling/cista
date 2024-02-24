#pragma once

namespace cista {

template <typename E>
void throw_exception(E&& e) {
#if !defined(__cpp_exceptions) || __cpp_exceptions < 199711L
  abort();
#else
  throw e;
#endif
}

}  // namespace cista