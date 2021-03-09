#ifndef NDNOB_LOG_HPP
#define NDNOB_LOG_HPP

#include "common.hpp"

#ifdef ARDUINO

#define NDNOB_LOG_FN(...) Serial.printf(__VA_ARGS__)
#define NDNOB_LOG_NOW millis()
#define NDNOB_LOG_FMT_NOW "%lu"

#else

#include <cinttypes>

#define NDNOB_LOG_FN(...) fprintf(stderr, ##__VA_ARGS__)
#define NDNOB_LOG_NOW (ndnph::port::UnixTime::now() / 1000)
#define NDNOB_LOG_FMT_NOW "%" PRIu64

#endif

/**
 * @brief Log a message to console.
 * @param cat message category.
 * @param fmt format string. It should end with "\n" if desired.
 */
#define NDNOB_LOG_MSG(cat, fmt, ...)                                                               \
  NDNOB_LOG_FN(NDNOB_LOG_FMT_NOW " [ndnob." cat "] " fmt, NDNOB_LOG_NOW, ##__VA_ARGS__)

/** @brief Log an error message to console. */
#define NDNOB_LOG_ERR(fmt, ...) NDNOB_LOG_MSG("E", fmt "\n", ##__VA_ARGS__)

/**
 * @brief Log state changes to console.
 * @param kind short string identifier.
 * @param value state variable.
 *
 * This macro contains a global variable, so it is unsuitable for per-instance state.
 */
#define NDNOB_LOG_STATE(kind, value)                                                               \
  __extension__({                                                                                  \
    static int prev = -1;                                                                          \
    bool changed = prev != (int)(value);                                                           \
    if (changed) {                                                                                 \
      NDNOB_LOG_MSG("S.%s", "%d\n", kind, (int)(value));                                           \
      prev = (int)(value);                                                                         \
    }                                                                                              \
    changed;                                                                                       \
  })

#endif // NDNOB_LOG_HPP
