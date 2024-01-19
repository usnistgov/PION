// SPDX-License-Identifier: NIST-PD

#ifndef SPAKE2_MBED_MBEDTLS_WRAPPERS_HPP
#define SPAKE2_MBED_MBEDTLS_WRAPPERS_HPP

#include "../common.hpp"
#include <mbedtls/entropy.h>

namespace mbed {

template<typename T, void (*InitFunc)(T*), void (*FreeFunc)(T*)>
class Object {
public:
  Object() noexcept {
    InitFunc(&m_obj);
  }

  ~Object() noexcept {
    FreeFunc(&m_obj);
  }

  T* operator->() noexcept {
    return &m_obj;
  }

  operator T*() noexcept {
    return &m_obj;
  }

  operator const T*() const noexcept {
    return &m_obj;
  }

private:
  T m_obj;
};

using Entropy = Object<mbedtls_entropy_context, mbedtls_entropy_init, mbedtls_entropy_free>;

} // namespace mbed

#endif // SPAKE2_MBED_MBEDTLS_WRAPPERS_HPP
