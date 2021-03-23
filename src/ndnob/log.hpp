#ifndef NDNOB_LOG_HPP
#define NDNOB_LOG_HPP

#include "common.hpp"

/** @brief Log an error message. */
#define NDNOB_LOG_ERR(fmt, ...) NDNPH_LOG_LINE("ndnob.E", fmt, ##__VA_ARGS__)

/**
 * @brief Log state changes.
 * @param kind short string identifier.
 * @param value state variable.
 *
 * This macro contains a global variable, so it is unsuitable for per-instance state.
 */
#define NDNOB_LOG_STATE(kind, value)                                                               \
  __extension__({                                                                                  \
    static int prev = -1;                                                                          \
    bool changed = prev != static_cast<int>(value);                                                \
    if (changed) {                                                                                 \
      NDNPH_LOG_LINE("ndnob.S.%s", "%d", kind, static_cast<int>(value));                           \
      prev = static_cast<int>(value);                                                              \
    }                                                                                              \
    changed;                                                                                       \
  })

#endif // NDNOB_LOG_HPP
