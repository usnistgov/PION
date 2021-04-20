// SPDX-License-Identifier: NIST-PD

#ifndef SPAKE2_MBED_MBEDTLS_WRAPPERS_HPP
#define SPAKE2_MBED_MBEDTLS_WRAPPERS_HPP

#include <mbedtls/bignum.h>
#include <mbedtls/ecp.h>

namespace mbed {

template<typename T,
         void (*InitFunc)(T*),
         void (*FreeFunc)(T*)>
class Object
{
public:
  Object() noexcept
  {
    InitFunc(&m_obj);
  }

  ~Object() noexcept
  {
    FreeFunc(&m_obj);
  }

  T* operator->() noexcept
  {
    return &m_obj;
  }

  operator T*() noexcept
  {
    return &m_obj;
  }

  operator const T*() const noexcept
  {
    return &m_obj;
  }

private:
  T m_obj;
};

class Mpi final : public Object<mbedtls_mpi,
                                mbedtls_mpi_init,
                                mbedtls_mpi_free>
{
public:
  using Object::Object;

  explicit Mpi(int i) noexcept
    : Mpi()
  {
    mbedtls_mpi_lset(*this, i);
  }

  Mpi(const Mpi& other) noexcept
    : Mpi()
  {
    mbedtls_mpi_copy(*this, other);
  }

  Mpi& operator=(const Mpi& other) noexcept
  {
    mbedtls_mpi_copy(*this, other);
    return *this;
  }
};

class EcPoint final : public Object<mbedtls_ecp_point,
                                    mbedtls_ecp_point_init,
                                    mbedtls_ecp_point_free>
{
  // TODO: add more helpers
};

} // namespace mbed

#endif // SPAKE2_MBED_MBEDTLS_WRAPPERS_HPP
