#ifndef NDNOB_LOG_HPP
#define NDNOB_LOG_HPP

#include "common.hpp"

#ifdef ARDUINO

#define NDNOB_LOG_FN(...) Serial.printf(__VA_ARGS__)
#define NDNOB_LOG_NOW millis()

#else

#define NDNOB_LOG_FN(...) fprintf(stderr, ##__VA_ARGS__)
#define NDNOB_LOG_NOW ndnph::port::UnixTime::now()

#endif

/**
 * @brief Log a message to console.
 * @param cat message category.
 * @param fmt format string. It should end with "\n" if desired.
 */
#define NDNOB_LOG_MSG(cat, fmt, ...)                                                               \
  NDNOB_LOG_FN("%lu [ndnob." cat "] " fmt, NDNOB_LOG_NOW, ##__VA_ARGS__)

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
  do {                                                                                             \
    static int prev = -1;                                                                          \
    if (prev != (int)(value)) {                                                                    \
      NDNOB_LOG_MSG("S.%s", "%d\n", kind, (int)(value));                                           \
      prev = (int)(value);                                                                         \
    }                                                                                              \
  } while (false)

#endif // NDNOB_LOG_HPP
