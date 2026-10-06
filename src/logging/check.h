#ifndef MINI_LOGGER_CHECK_H_
#define MINI_LOGGER_CHECK_H_

// clang-format off
#include "logging_base.h"

#include <intrin.h> // Keep this below logging_base.h

// DCHECK is a debug-only assertion macro. It checks if the condition is true,
// and if not, triggers a debug break. It is only evaluated when DCHECK_ON is true
//
// Usage:
//   DCHECK(ptr != nullptr);
//   DCHECK(index < array_size);
//   DCHECK(number >= int);

// Macro to convert to string
#ifndef _STRINGIZER
 #define _STRINGIZER(in) #in
 #define STRINGIZE(in) _STRINGIZER(in)
#endif // _STRINGIZER
// clang-format on

namespace logging {

  // Intentionally kill app, don't use __debugbreak, it isn't reliable.
  void KillApp();

  // Function that runs LOG(FATAL)
  void CheckImpl(const char* func_sig, const int line_num, const char* condition, bool check_flag);

  // for NOTREACHED()
  void NotReachedImpl(const char* func_sig, const int line_num);

} // namespace logging

// Same as DCHECK, but is always on. Use when a condition should be fatal if false.
#define CHECK(condition) logging::CheckImpl(__func__, __LINE__, STRINGIZE(condition), !(condition))

#ifdef DCHECK_ON
 #define DCHECK(condition) CHECK(condition)
#else // is_dcheck not set, DCHECKs are no-ops
 #define DCHECK(condition) ((void)0)
#endif // DCHECK

#define NOTREACHED() logging::NotReachedImpl(__func__, __LINE__)

#endif // MINI_LOGGER_CHECK_H_
