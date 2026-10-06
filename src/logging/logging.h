#ifndef MINI_LOGGER_LOGGING_H_
#define MINI_LOGGER_LOGGING_H_

#include "check.h"
#include "file_util.h"
#include "logging_base.h"

enum LogLevel {
  LOG_INFO     = 0,
  LOG_DEBUG    = 1,
  LOG_WARN     = 2,
  LOG_ERROR    = 3,
  LOG_FATAL    = 4,
  LOG_VERBOSE  = 5,
  MAX_LOGLEVEL = 6
};

// Must have 0x2 to set bit 29 for custom user error codes.
static const DWORD ERROR_MAX_LOGLEVEL = 0x20000006;

// Toggle to test LOG(FATAL) which will crash the app
static constexpr bool test_fatal = false;

namespace logging {

  // Stores logging settings to be used for initialization
  typedef struct {
    LogDest log_sink;           // Where to log to
    std::wstring logfile_name;  // Name of log file
    std::wstring app_name;      // Name of app using this library
    bool show_func_sigs;        // Whether to prepend the function name to log lines
    bool show_line_numbers;     // Whether to prepend line numbers
    bool show_time;             // Whether to prepend the UTC time
    LogLevel full_prefix_level; // What log level to show all prefixes
  } LogInitSettings;

  // Tags any integer value as "format me in hex". The LogMessage stream
  // operator picks up the wrapper regardless of the value's underlying
  // type, so this works for DWORD, QWORD, HRESULT, bitmasks, addresses -
  // anything where 0xDEADBEEF reads better than 3735928559. We can't
  // overload directly on DWORD/QWORD because they're typedefs for
  // unsigned long / unsigned long long, which already have decimal
  // overloads, and C++ overload resolution sees through typedefs.
  //   LOG(ERROR) << L"GetLastError: " << logging::Hex(GetLastError());
  // The explicit constructor is what makes `Hex(value)` work under C++17
  // (parenthesized aggregate init is C++20-only). With it, the compiler
  // also synthesizes the deduction guide automatically, so callers don't
  // need to spell out the template argument.
  template <typename T>
  struct Hex {
    T value;
    explicit Hex(T v) : value(v) {}
  };

  class LogMessage {
   public:
    explicit LogMessage(LogLevel level,
                        bool log_to_file,
                        bool log_to_console,
                        const char* func_sig,
                        int line_number); // Explicit constructor
    ~LogMessage(); // Actual logging happens on destruction, simple lazy logger

    LogMessage(const LogMessage&)            = delete;
    LogMessage& operator=(const LogMessage&) = delete;

    // Narrow string/char overloads - convert to wide
    LogMessage& operator<<(char value);
    LogMessage& operator<<(const char* value);

    // Wide string/char overloads - stream directly
    LogMessage& operator<<(wchar_t value);
    LogMessage& operator<<(const wchar_t* value);
    LogMessage& operator<<(const std::string& value);
    LogMessage& operator<<(const std::wstring& value);

    // Numeric type overloads
    LogMessage& operator<<(int value);
    LogMessage& operator<<(unsigned int value);
    LogMessage& operator<<(long value);
    LogMessage& operator<<(unsigned long value);
    LogMessage& operator<<(long long value);
    LogMessage& operator<<(unsigned long long value);
    LogMessage& operator<<(float value);
    LogMessage& operator<<(double value);
    LogMessage& operator<<(long double value);

    // Other Win32 overloads
    LogMessage& operator<<(HWND value);

    // Hex-formatted integer (opt-in via the logging::Hex wrapper above).
    // Prints with a 0x prefix and restores the stream's decimal mode after.
    // Cast to unsigned long long so char-shaped values don't print as a
    // single character, mirroring the HWND overload's casting style.
    template <typename T>
    LogMessage& operator<<(Hex<T> h) {
      stream_ << std::showbase << std::hex << static_cast<unsigned long long>(h.value) << std::dec
              << std::noshowbase;
      return *this;
    }

    // Generic template for streams and other types (manipulators, etc.)
    template <typename T>
    LogMessage& operator<<(const T& value) {
      stream_ << value;
      return *this;
    }

   protected:
    bool IsDCheck(); // Whether to use DCHECK

   private:
    // Which logging level to use
    LogLevel level_;
    // Whether to append stream to log file
    bool log_to_file_;
    // Whether to send stream to stdout/stderr
    bool log_to_console_;
    // Function name of the call site (from __func__)
    const char* func_sig_;
    // Source line number of the call site (from __LINE__)
    int line_number_;
    // The stream itself
    std::wostringstream stream_;
  };

  extern volatile bool dcheck_log_;

  // Initialize logging for this program
  bool InitLogging(HINSTANCE hInstance, LogInitSettings InitSettings);

  // Call to clean up logging stream and any file handles
  bool DeInitLogging(HINSTANCE hInstance);

  // Set whether to use DLOG
  void SetIsDCheck(bool set_is_dcheck);

  // Test that logging works as expected.
  void TestLogging();

} // namespace logging

// Regular logging
#define LOG(level) logging::LogMessage(LOG_##level, true, true, __func__, __LINE__)

// Log only to console
#define CLOG(level) logging::LogMessage(LOG_##level, false, true, __func__, __LINE__)

// Log only to file
#define FLOG(level) logging::LogMessage(LOG_##level, true, false, __func__, __LINE__)

// Debug logging
#define DLOG() logging::LogMessage(LOG_DEBUG, true, true, __func__, __LINE__)

// Verbose logging
#define VLOG() logging::LogMessage(LOG_VERBOSE, true, true, __func__, __LINE__)

#endif // MINI_LOGGER_LOGGING_H_
