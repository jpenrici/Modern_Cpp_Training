// Statistic/native/include/statistic/stats_c_api.h

/**
 * @file stats_c_api.h
 * @brief Public C API of statslib (the ONLY header meant for Kotlin/Native cinterop).
 *
 * Design rules for this boundary:
 *  - Pure C: no C++ types, no exceptions, no templates.
 *  - Fixed-width integer types, so the layout is identical on both sides.
 *  - Every function returns a status code; results go through out-parameters.
 *  - Nothing is allocated by the library, so there is nothing to free.
 *
 * Linux only (GCC 14).
 */
#ifndef STATISTIC_STATS_C_API_H
#define STATISTIC_STATS_C_API_H

#include <stdint.h>

/* Export marker. The library is built with -fvisibility=hidden, so only
 * symbols tagged with STATS_API are visible to the dynamic linker. */
#define STATS_API __attribute__((visibility("default")))

#ifdef __cplusplus
extern "C" {
#endif

/** Status codes returned by the functions of this API. */
typedef enum StatsStatus {
    STATS_OK = 0, /**< Success. */
    STATS_ERR_NULL_ARGUMENT = 1, /**< `data` or `out` is NULL. */
    STATS_ERR_INVALID_SIZE = 2, /**< `size` is negative. */
    STATS_ERR_EMPTY_INPUT = 3, /**< `size` is zero. */
    STATS_ERR_NON_FINITE = 4, /**< Input contains NaN or +/-infinity. */
    STATS_ERR_OVERFLOW = 5, /**< A result does not fit in a double. */
    STATS_ERR_INTERNAL = 6 /**< Unexpected failure (e.g. out of memory). */
} StatsStatus;

/** Summary statistics. Four plain doubles: 32 bytes, no padding. */
typedef struct StatsResult {
    double mean;
    double median;
    double variance; /**< POPULATION variance (divides by n). */
    double stddev; /**< sqrt(variance). */
} StatsResult;

/**
 * @brief Computes mean, median, variance and standard deviation of a flat vector.
 *
 * @param data Pointer to `size` contiguous doubles. Not modified, not retained.
 * @param size Number of elements (must be > 0).
 * @param out  Receives the result. It is zeroed first, so it is never left
 *             uninitialised, even when an error status is returned.
 * @return STATS_OK on success, otherwise the reason of the failure.
 *         This function never throws and never terminates the process.
 */
STATS_API StatsStatus stats_compute(const double* data, int64_t size, StatsResult* out);

/**
 * @brief Human-readable description of a status code.
 * @return A static, NUL-terminated string. Never NULL, must NOT be freed.
 */
STATS_API const char* stats_status_message(StatsStatus status);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* STATISTIC_STATS_C_API_H */
