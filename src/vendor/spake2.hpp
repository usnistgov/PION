// SPDX-License-Identifier: NIST-PD

#ifndef SPAKE2_MBED_SPAKE2_HPP
#define SPAKE2_MBED_SPAKE2_HPP

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

#include <mbedtls/entropy.h>
#include <mbedtls/hkdf.h>
#include <mbedtls/hmac_drbg.h>
#include <mbedtls/md.h>
#include <mbedtls/ssl_internal.h> // for mbedtls_ssl_safer_memcmp

#include "mbedtls-wrappers.hpp"

#ifdef SPAKE2_DEBUG
#include <iostream>
#define SPAKE2_MBED_ERR(x) \
  do { \
    std::cerr << __FILE__ << ':' << __LINE__ << ": mbedtls error " << (x) << '\n'; \
  } while (false)
#else
#define SPAKE2_MBED_ERR(x) do {} while (false)
#endif

namespace spake2 {

enum class Role {
  Alice,
  Bob,
};

namespace detail {

// std::max is not constexpr in C++11
template<typename T>
constexpr const T&
max(const T& a, const T& b)
{
  return a < b ? b : a;
}

void
appendToTranscript(std::vector<uint8_t>& transcript, const uint8_t* buf, size_t buflen);

class ContextBase
{
protected:
  explicit
  ContextBase(Role role) noexcept;

  // Protocol state machine: each party goes through each of these states in order
  enum class State {
    Initial,
    AwaitingPublicShare,
    SendingConfirmation,
    AwaitingConfirmation,
    Done,
  };

  const Role m_role;
  State m_state = State::Initial;

  static const mbed::Mpi s_one;
  static const mbed::Mpi s_minusOne;
};

} // namespace detail

struct P256
{
  static constexpr mbedtls_ecp_group_id Id = MBEDTLS_ECP_DP_SECP256R1;
  enum {
    ScalarSize = 32,
    UncompressedPointSize = 65,
  };

  static const uint8_t M[UncompressedPointSize];
  static const uint8_t N[UncompressedPointSize];
};

struct P384
{
  static constexpr mbedtls_ecp_group_id Id = MBEDTLS_ECP_DP_SECP384R1;
  enum {
    ScalarSize = 48,
    UncompressedPointSize = 97,
  };

  static const uint8_t M[UncompressedPointSize];
  static const uint8_t N[UncompressedPointSize];
};

struct P521
{
  static constexpr mbedtls_ecp_group_id Id = MBEDTLS_ECP_DP_SECP521R1;
  enum {
    ScalarSize = 66,
    UncompressedPointSize = 133,
  };

  static const uint8_t M[UncompressedPointSize];
  static const uint8_t N[UncompressedPointSize];
};

struct SHA256
{
  static constexpr mbedtls_md_type_t Type = MBEDTLS_MD_SHA256;
  enum {
    OutputSize = 32,
  };
};

struct SHA512
{
  static constexpr mbedtls_md_type_t Type = MBEDTLS_MD_SHA512;
  enum {
    OutputSize = 64,
  };
};

/**
 * @brief This class represents an execution of the SPAKE2 protocol (draft-18).
 *
 * SPAKE2 is a protocol for two parties that share a password to derive a strong shared
 * key with no risk of disclosing the password. The password can be low entropy.
 *
 * @sa https://tools.ietf.org/html/draft-irtf-cfrg-spake2-18
 */
template<typename Group = P256,
         typename Hash = SHA256>
class Context final : detail::ContextBase
{
public:
  enum {
    FirstMessageSize  = Group::UncompressedPointSize,
    SecondMessageSize = Hash::OutputSize,
    SharedKeySize     = Hash::OutputSize / 2,
  };

  explicit
  Context(Role role, mbedtls_entropy_context* entropyCtx) noexcept;

  bool start(const uint8_t* pw, size_t pwLen,
             const uint8_t* myId = nullptr, size_t myIdLen = 0,
             const uint8_t* peerId = nullptr, size_t peerIdLen = 0,
             const uint8_t* aad = nullptr, size_t aadLen = 0) noexcept;

  bool generateFirstMessage(uint8_t* outMsg, size_t outMsgLen) noexcept;

  bool processFirstMessage(const uint8_t* inMsg, size_t inMsgLen) noexcept;

  bool generateSecondMessage(uint8_t* outMsg, size_t outMsgLen) noexcept;

  bool processSecondMessage(const uint8_t* inMsg, size_t inMsgLen) noexcept;

  /**
   * @brief Returns the shared key established by the SPAKE2 exchange.
   * @pre Can only be called after processSecondMessage() returns true.
   */
  const std::array<uint8_t, SharedKeySize>& getSharedKey() const noexcept
  {
    assert(m_state == State::Done);
    return m_key;
  }

  // for testing only
  void setw(const char* w) noexcept
  {
    int ret = mbedtls_mpi_read_string(m_w, 16, w);
    assert(ret == 0);
  }

  // for testing only
  void setx(const char* x) noexcept
  {
    int ret = mbedtls_mpi_read_string(m_x, 16, x);
    assert(ret == 0);
  }

private:
  std::array<uint8_t, detail::max(FirstMessageSize, SecondMessageSize)> m_myMsg{};
  std::array<uint8_t, Hash::OutputSize> m_expectedMac{};
  std::array<uint8_t, SharedKeySize> m_key{};

  mbed::Object<mbedtls_hmac_drbg_context, mbedtls_hmac_drbg_init, mbedtls_hmac_drbg_free> m_drbg;
  mbed::Object<mbedtls_md_context_t, mbedtls_md_init, mbedtls_md_free> m_md;
  mbed::Object<mbedtls_ecp_group, mbedtls_ecp_group_init, mbedtls_ecp_group_free> m_group;

  mbed::Mpi m_w;
  mbed::Mpi m_x;
  mbed::EcPoint m_S;
  mbed::EcPoint m_M;
  mbed::EcPoint m_N;

  std::vector<uint8_t> m_transcript;    // TODO: consider making statically sized
  std::vector<uint8_t> m_info{          // TODO: consider making statically sized
    'C', 'o', 'n', 'f', 'i', 'r', 'm', 'a', 't', 'i', 'o', 'n', 'K', 'e', 'y', 's',
  };
};

template<typename Group, typename Hash>
Context<Group, Hash>::Context(Role role, mbedtls_entropy_context* entropyCtx) noexcept
  : ContextBase(role)
{
  assert(entropyCtx != nullptr);

  auto mdInfo = mbedtls_md_info_from_type(Hash::Type);
  assert(mdInfo != nullptr);

  // Initialize HMAC_DRBG context
  int ret = mbedtls_hmac_drbg_seed(m_drbg, mdInfo, mbedtls_entropy_func, entropyCtx, nullptr, 0);
  assert(ret == 0);

  // Initialize digest context
  ret = mbedtls_md_setup(m_md, mdInfo, 1);
  assert(ret == 0);

  // Initialize EC group
  ret = mbedtls_ecp_group_load(m_group, Group::Id);
  assert(ret == 0);

  // Load protocol constants M and N
  ret = mbedtls_ecp_point_read_binary(m_group, m_M, Group::M, sizeof(Group::M));
  assert(ret == 0);
  ret = mbedtls_ecp_point_read_binary(m_group, m_N, Group::N, sizeof(Group::N));
  assert(ret == 0);
}

template<typename Group, typename Hash>
bool
Context<Group, Hash>::start(const uint8_t* pw, size_t pwLen,
                            const uint8_t* myId, size_t myIdLen,
                            const uint8_t* peerId, size_t peerIdLen,
                            const uint8_t* aad, size_t aadLen) noexcept
{
  // TODO: sanity-check state machine?

  // Allocate memory for the transcript and copy the identities
  m_transcript.reserve(sizeof(uint64_t) * 6 +             // lengths
                       myIdLen + peerIdLen +              // identities
                       Group::UncompressedPointSize * 3 + // S, T, K (points)
                       Group::ScalarSize);                // w (scalar)
  if (m_role == Role::Alice) {
    detail::appendToTranscript(m_transcript, myId, myIdLen);
    detail::appendToTranscript(m_transcript, peerId, peerIdLen);
  }
  else {
    detail::appendToTranscript(m_transcript, peerId, peerIdLen);
    detail::appendToTranscript(m_transcript, myId, myIdLen);
  }

  // Append the Additional Authenticated Data (AAD) to the KDF info string
  m_info.insert(m_info.end(), aad, aad + aadLen);

  // Calculate the hash of the user-supplied password pw
  std::array<uint8_t, Hash::OutputSize> pwHash{};
  int ret = mbedtls_md_starts(m_md);
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }
  ret = mbedtls_md_update(m_md, pw, pwLen);
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }
  ret = mbedtls_md_finish(m_md, pwHash.data());
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }

  mbed::Mpi pwHashScalar;
  ret = mbedtls_mpi_read_binary(pwHashScalar, pwHash.data(), pwHash.size());
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }
  ret = mbedtls_mpi_mod_mpi(m_w, pwHashScalar, &m_group->N);
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }

  // Generate random scalar x
  mbed::Mpi random;
  // NOTE: generate 8 extra bytes to avoid bias in modulo operation
  ret = mbedtls_mpi_fill_random(random, Group::ScalarSize + 8, mbedtls_hmac_drbg_random, m_drbg);
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }
  ret = mbedtls_mpi_mod_mpi(m_x, random, &m_group->N);
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }

  return true;
}

template<typename Group, typename Hash>
bool
Context<Group, Hash>::generateFirstMessage(uint8_t* outMsg, size_t outMsgLen) noexcept
{
  if (m_state != State::Initial) {
    return false;
  }

  // NOTE: the two mbedtls_ecp_mul() calls below are not combined into the mbedtls_ecp_muladd()
  //       that follows because the latter is _not_ constant time while the former is.

  int ret;
  mbed::EcPoint X;
  // X = x * P
  ret = mbedtls_ecp_mul(m_group, X, m_x, &m_group->G, mbedtls_hmac_drbg_random, m_drbg);
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }

  mbed::EcPoint wMN;
  // wMN = w * (M|N)
  ret = mbedtls_ecp_mul(m_group, wMN, m_w, m_role == Role::Alice ? m_M : m_N,
                        mbedtls_hmac_drbg_random, m_drbg);
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }

  // S = wMN + X
  ret = mbedtls_ecp_muladd(m_group, m_S, s_one, wMN, s_one, X);
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }

  std::array<uint8_t, Group::UncompressedPointSize> binS{};
  size_t lenS = 0;
  ret = mbedtls_ecp_point_write_binary(m_group, m_S, MBEDTLS_ECP_PF_UNCOMPRESSED,
                                       &lenS, binS.data(), binS.size());
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }

  std::memcpy(m_myMsg.data(), binS.data(), lenS);
  // TODO: sanity checks
  std::memcpy(outMsg, m_myMsg.data(), outMsgLen);

  m_state = State::AwaitingPublicShare;
  return true;
}

template<typename Group, typename Hash>
bool
Context<Group, Hash>::processFirstMessage(const uint8_t* inMsg, size_t inMsgLen) noexcept
{
  if (m_state != State::AwaitingPublicShare) {
    return false;
  }

  int ret;
  mbed::EcPoint T;
  ret = mbedtls_ecp_point_read_binary(m_group, T, inMsg, inMsgLen);
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }
  // Verify that the received point is on the curve
  ret = mbedtls_ecp_check_pubkey(m_group, T);
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }

  mbed::EcPoint wNM;
  // wNM = w * (N|M)
  ret = mbedtls_ecp_mul(m_group, wNM, m_w, m_role == Role::Alice ? m_N : m_M,
                        mbedtls_hmac_drbg_random, m_drbg);
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }

  mbed::EcPoint Y;
  // Y = T - wNM
  ret = mbedtls_ecp_muladd(m_group, Y, s_one, T, s_minusOne, wNM);
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }

  mbed::EcPoint K;
  // K = h * x * Y
  // NOTE: the cofactor h is 1 for NIST curves
  ret = mbedtls_ecp_mul(m_group, K, m_x, Y, mbedtls_hmac_drbg_random, m_drbg);
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }

  std::array<uint8_t, Group::UncompressedPointSize> binK{};
  size_t lenK = 0;
  ret = mbedtls_ecp_point_write_binary(m_group, K, MBEDTLS_ECP_PF_UNCOMPRESSED,
                                       &lenK, binK.data(), binK.size());
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }

  std::array<uint8_t, Group::ScalarSize> binW{};
  ret = mbedtls_mpi_write_binary(m_w, binW.data(), binW.size());
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }

  // Finalize protocol transcript
  if (m_role == Role::Alice) {
    detail::appendToTranscript(m_transcript, m_myMsg.data(), FirstMessageSize);
    detail::appendToTranscript(m_transcript, inMsg, inMsgLen);
  }
  else {
    detail::appendToTranscript(m_transcript, inMsg, inMsgLen);
    detail::appendToTranscript(m_transcript, m_myMsg.data(), FirstMessageSize);
  }
  detail::appendToTranscript(m_transcript, binK.data(), lenK);
  detail::appendToTranscript(m_transcript, binW.data(), binW.size());

  // Calculate the hash of the transcript
  std::array<uint8_t, Hash::OutputSize> transcriptHash{};
  ret = mbedtls_md_starts(m_md);
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }
  ret = mbedtls_md_update(m_md, m_transcript.data(), m_transcript.size());
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }
  ret = mbedtls_md_finish(m_md, transcriptHash.data());
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }

  const uint8_t* Ke = transcriptHash.data();
  const uint8_t* Ka = transcriptHash.data() + transcriptHash.size() / 2;
  std::memcpy(m_key.data(), Ke, m_key.size());

  // Derive confirmation keys (HKDF)
  std::array<uint8_t, Hash::OutputSize> Kc{};
  ret = mbedtls_hkdf(mbedtls_md_info_from_type(Hash::Type), nullptr, 0,
                     Ka, transcriptHash.size() / 2, m_info.data(), m_info.size(), Kc.data(), Kc.size());
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }

  const uint8_t* KcA = Kc.data();
  const uint8_t* KcB = Kc.data() + Kc.size() / 2;

  // Construct confirmation messages (HMAC)
  std::array<uint8_t, Hash::OutputSize> macA{};
  ret = mbedtls_md_hmac_starts(m_md, KcA, Kc.size() / 2);
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }
  ret = mbedtls_md_hmac_update(m_md, m_transcript.data(), m_transcript.size());
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }
  ret = mbedtls_md_hmac_finish(m_md, macA.data());
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }
  std::array<uint8_t, Hash::OutputSize> macB{};
  ret = mbedtls_md_hmac_starts(m_md, KcB, Kc.size() / 2);
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }
  ret = mbedtls_md_hmac_update(m_md, m_transcript.data(), m_transcript.size());
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }
  ret = mbedtls_md_hmac_finish(m_md, macB.data());
  if (ret != 0) {
    SPAKE2_MBED_ERR(ret);
    return false;
  }

  if (m_role == Role::Alice) {
    std::memcpy(m_myMsg.data(), macA.data(), macA.size());
    std::memcpy(m_expectedMac.data(), macB.data(), macB.size());
  }
  else {
    std::memcpy(m_myMsg.data(), macB.data(), macB.size());
    std::memcpy(m_expectedMac.data(), macA.data(), macA.size());
  }

  m_state = State::SendingConfirmation;
  return true;
}

template<typename Group, typename Hash>
bool
Context<Group, Hash>::generateSecondMessage(uint8_t* outMsg, size_t outMsgLen) noexcept
{
  if (m_state != State::SendingConfirmation) {
    return false;
  }

  // TODO: sanity checks
  std::memcpy(outMsg, m_myMsg.data(), outMsgLen);

  m_state = State::AwaitingConfirmation;
  return true;
}

template<typename Group, typename Hash>
bool
Context<Group, Hash>::processSecondMessage(const uint8_t* inMsg, size_t inMsgLen) noexcept
{
  if (m_state != State::AwaitingConfirmation) {
    return false;
  }

  // Check that the peer's MAC matches the one we expect
  if (inMsgLen != m_expectedMac.size()) {
    return false;
  }
  // TODO: mbedtls_ssl_safer_memcmp is an internal function, replace it with something else
  if (mbedtls_ssl_safer_memcmp(inMsg, m_expectedMac.data(), m_expectedMac.size()) != 0) {
    return false;
  }

  m_state = State::Done;
  return true;
}

} // namespace spake2

#endif // SPAKE2_MBED_SPAKE2_HPP
