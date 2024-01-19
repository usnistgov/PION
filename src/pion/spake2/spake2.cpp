// SPDX-License-Identifier: NIST-PD

#include "spake2.hpp"

#include <endian.h>

namespace spake2 {
namespace detail {

void
appendToTranscript(std::vector<uint8_t>& transcript, const uint8_t* buf, size_t buflen) {
  uint64_t len = htole64(buflen);
  auto lenptr = reinterpret_cast<const uint8_t*>(&len);
  transcript.insert(transcript.end(), lenptr, lenptr + sizeof(len));
  transcript.insert(transcript.end(), buf, buf + buflen);
}

const ndnph::mbedtls::Mpi ContextBase::s_one{1};
const ndnph::mbedtls::Mpi ContextBase::s_minusOne{-1};

} // namespace detail

const uint8_t P256::M[] = {
  0x04, 0x88, 0x6e, 0x2f, 0x97, 0xac, 0xe4, 0x6e, 0x55, 0xba, 0x9d, 0xd7, 0x24,
  0x25, 0x79, 0xf2, 0x99, 0x3b, 0x64, 0xe1, 0x6e, 0xf3, 0xdc, 0xab, 0x95, 0xaf,
  0xd4, 0x97, 0x33, 0x3d, 0x8f, 0xa1, 0x2f, 0x5f, 0xf3, 0x55, 0x16, 0x3e, 0x43,
  0xce, 0x22, 0x4e, 0x0b, 0x0e, 0x65, 0xff, 0x02, 0xac, 0x8e, 0x5c, 0x7b, 0xe0,
  0x94, 0x19, 0xc7, 0x85, 0xe0, 0xca, 0x54, 0x7d, 0x55, 0xa1, 0x2e, 0x2d, 0x20,
};
const uint8_t P256::N[] = {
  0x04, 0xd8, 0xbb, 0xd6, 0xc6, 0x39, 0xc6, 0x29, 0x37, 0xb0, 0x4d, 0x99, 0x7f,
  0x38, 0xc3, 0x77, 0x07, 0x19, 0xc6, 0x29, 0xd7, 0x01, 0x4d, 0x49, 0xa2, 0x4b,
  0x4f, 0x98, 0xba, 0xa1, 0x29, 0x2b, 0x49, 0x07, 0xd6, 0x0a, 0xa6, 0xbf, 0xad,
  0xe4, 0x50, 0x08, 0xa6, 0x36, 0x33, 0x7f, 0x51, 0x68, 0xc6, 0x4d, 0x9b, 0xd3,
  0x60, 0x34, 0x80, 0x8c, 0xd5, 0x64, 0x49, 0x0b, 0x1e, 0x65, 0x6e, 0xdb, 0xe7,
};

const uint8_t P384::M[] = {
  0x04, 0x0f, 0xf0, 0x89, 0x5a, 0xe5, 0xeb, 0xf6, 0x18, 0x70, 0x80, 0xa8, 0x2d, 0x82,
  0xb4, 0x2e, 0x27, 0x65, 0xe3, 0xb2, 0xf8, 0x74, 0x9c, 0x7e, 0x05, 0xeb, 0xa3, 0x66,
  0x43, 0x4b, 0x36, 0x3d, 0x3d, 0xc3, 0x6f, 0x15, 0x31, 0x47, 0x39, 0x07, 0x4d, 0x2e,
  0xb8, 0x61, 0x3f, 0xce, 0xec, 0x28, 0x53, 0x97, 0x59, 0x2c, 0x55, 0x79, 0x7c, 0xdd,
  0x77, 0xc0, 0x71, 0x5c, 0xb7, 0xdf, 0x21, 0x50, 0x22, 0x0a, 0x01, 0x19, 0x86, 0x64,
  0x86, 0xaf, 0x42, 0x34, 0xf3, 0x90, 0xaa, 0xd1, 0xf6, 0xad, 0xdd, 0xe5, 0x93, 0x09,
  0x09, 0xad, 0xc6, 0x7a, 0x1f, 0xc0, 0xc9, 0x9b, 0xa3, 0xd5, 0x2d, 0xc5, 0xdd,
};
const uint8_t P384::N[] = {
  0x04, 0xc7, 0x2c, 0xf2, 0xe3, 0x90, 0x85, 0x3a, 0x1c, 0x1c, 0x4a, 0xd8, 0x16, 0xa6,
  0x2f, 0xd1, 0x58, 0x24, 0xf5, 0x60, 0x78, 0x91, 0x8f, 0x43, 0xf9, 0x22, 0xca, 0x21,
  0x51, 0x8f, 0x9c, 0x54, 0x3b, 0xb2, 0x52, 0xc5, 0x49, 0x02, 0x14, 0xcf, 0x9a, 0xa3,
  0xf0, 0xba, 0xab, 0x4b, 0x66, 0x5c, 0x10, 0xc3, 0x8b, 0x7d, 0x7f, 0x4e, 0x7f, 0x32,
  0x03, 0x17, 0xcd, 0x71, 0x73, 0x15, 0xa7, 0x97, 0xc7, 0xe0, 0x29, 0x33, 0xae, 0xf6,
  0x8b, 0x36, 0x4c, 0xbf, 0x84, 0xeb, 0xc6, 0x19, 0xbe, 0xdb, 0xe2, 0x1f, 0xf5, 0xc6,
  0x9e, 0xa0, 0xf1, 0xfe, 0xd5, 0xd7, 0xe3, 0x20, 0x04, 0x18, 0x07, 0x3f, 0x40,
};

const uint8_t P521::M[] = {
  0x04, 0x00, 0x3f, 0x06, 0xf3, 0x81, 0x31, 0xb2, 0xba, 0x26, 0x00, 0x79, 0x1e, 0x82, 0x48,
  0x8e, 0x8d, 0x20, 0xab, 0x88, 0x9a, 0xf7, 0x53, 0xa4, 0x18, 0x06, 0xc5, 0xdb, 0x18, 0xd3,
  0x7d, 0x85, 0x60, 0x8c, 0xfa, 0xe0, 0x6b, 0x82, 0xe4, 0xa7, 0x2c, 0xd7, 0x44, 0xc7, 0x19,
  0x19, 0x35, 0x62, 0xa6, 0x53, 0xea, 0x1f, 0x11, 0x9e, 0xef, 0x93, 0x56, 0x90, 0x7e, 0xdc,
  0x9b, 0x56, 0x97, 0x99, 0x62, 0xd7, 0xaa, 0x01, 0xbd, 0xd1, 0x79, 0xa3, 0xd5, 0x47, 0x61,
  0x08, 0x92, 0xe9, 0xb9, 0x6d, 0xea, 0x1e, 0xab, 0x10, 0xbd, 0xd7, 0xac, 0x5a, 0xe0, 0xcf,
  0x75, 0xaa, 0x0f, 0x85, 0x3b, 0xfd, 0x18, 0x5c, 0xf7, 0x82, 0xf8, 0x94, 0x30, 0x19, 0x98,
  0xb1, 0x1d, 0x18, 0x98, 0xed, 0xe2, 0x70, 0x1d, 0xca, 0x37, 0xa2, 0xbb, 0x50, 0xb4, 0xf5,
  0x19, 0xc3, 0xd8, 0x9a, 0x7d, 0x05, 0x4b, 0x51, 0xfb, 0x84, 0x91, 0x21, 0x92,
};
const uint8_t P521::N[] = {
  0x04, 0x00, 0xc7, 0x92, 0x4b, 0x9e, 0xc0, 0x17, 0xf3, 0x09, 0x45, 0x62, 0x89, 0x43, 0x36,
  0xa5, 0x3c, 0x50, 0x16, 0x7b, 0xa8, 0xc5, 0x96, 0x38, 0x76, 0x88, 0x05, 0x42, 0xbc, 0x66,
  0x9e, 0x49, 0x4b, 0x25, 0x32, 0xd7, 0x6c, 0x5b, 0x53, 0xdf, 0xb3, 0x49, 0xfd, 0xf6, 0x91,
  0x54, 0xb9, 0xe0, 0x04, 0x8c, 0x58, 0xa4, 0x2e, 0x8e, 0xd0, 0x4c, 0xef, 0x05, 0x2a, 0x3b,
  0xc3, 0x49, 0xd9, 0x55, 0x75, 0xcd, 0x25, 0x01, 0xc6, 0x2b, 0xee, 0x65, 0x0c, 0x92, 0x87,
  0xa6, 0x51, 0xbb, 0x75, 0xc7, 0xf3, 0x9a, 0x20, 0x06, 0x87, 0x33, 0x47, 0xb7, 0x69, 0x84,
  0x0d, 0x26, 0x1d, 0x17, 0x76, 0x0b, 0x10, 0x7e, 0x29, 0xf0, 0x91, 0xd5, 0x56, 0xa8, 0x2a,
  0x2e, 0x4c, 0xde, 0x0c, 0x40, 0xb8, 0x4b, 0x95, 0xb8, 0x78, 0xdb, 0x24, 0x89, 0xef, 0x76,
  0x02, 0x06, 0x42, 0x4b, 0x3f, 0xe7, 0x96, 0x8a, 0xa8, 0xe0, 0xb1, 0xf3, 0x34,
};

} // namespace spake2
