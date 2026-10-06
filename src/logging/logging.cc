#include "logging.h"

#include <mutex>

namespace logging {
  volatile bool dcheck_log_     = false;
  bool logging_initialized      = false;
  std::wstring kProgName        = L"";
  std::wstring kLogFileName     = L"";
  static bool show_func_sigs    = false;
  static bool show_line_numbers = true;
  static bool show_time         = false;
  // What minimum log level to show all prefixes,
  // for example FATAL to help diagnose errors.
  LogLevel full_prefix_level = LOG_FATAL;
} // namespace logging

logging::LogMessage::LogMessage(LogLevel level,
                                bool log_to_file,
                                bool log_to_console,
                                const char* func_sig,
                                int line_number)
    : level_(level),
      log_to_file_(log_to_file),
      log_to_console_(log_to_console),
      func_sig_(func_sig),
      line_number_(line_number) {
}

logging::LogMessage::~LogMessage() {
  if (!logging_initialized) {
    OutputDebugStringW(L"Logging not initialized!");
    return;
  }

  const bool show_full_prefix = (level_ >= full_prefix_level) || (level_ == LOG_VERBOSE);
  const wchar_t* level_prefix;
  switch (level_) {
    case LOG_INFO:
      level_prefix = L"[INFO] ";
      break;
    case LOG_DEBUG:
      level_prefix = IsDCheck() ? L"[DCHECK] " : L"[DEBUG] ";
      break;
    case LOG_WARN:
      level_prefix = L"[WARN] ";
      break;
    case LOG_ERROR:
      level_prefix = L"[ERROR] ";
      break;
    case LOG_FATAL:
      level_prefix = L"[FATAL] ";
      break;
    case LOG_VERBOSE:
      level_prefix = L"[VERBOSE] ";
      break;
    case MAX_LOGLEVEL:
      std::wcerr << ToWide(__func__) << L"MAX LOGLEVEL" << std::endl;
      SetLastErrorEx(ERROR_MAX_LOGLEVEL, SLE_ERROR);
      return;
    default:
      std::wcerr << ToWide(__func__) << L"INVALID LOG LEVEL" << std::endl;
      NOTREACHED();
      return;
  }

  std::wstring full_prefix;
  if (show_time || show_full_prefix) {
    SYSTEMTIME systime;
    GetSystemTime(&systime);
    std::wostringstream ts;
    ts << std::setfill(L'0')
       << L'T'
       /* << std::setw(4) << systime.wYear  << L'-'
          << std::setw(2) << systime.wMonth << L'-'
          << std::setw(2) << systime.wDay   << L'|' */
       << std::setw(2) << systime.wHour << L':' << std::setw(2) << systime.wMinute << L':'
       << std::setw(2) << systime.wSecond << L' ';
    full_prefix += ts.str();
  }
  if ((show_func_sigs && func_sig_) || show_full_prefix) {
    const std::wstring func_str = ToWide(func_sig_);
    full_prefix += func_str;
    if (func_str.empty() || func_str.back() != L')') {
      full_prefix += L"()";
    }
    if (show_line_numbers || show_full_prefix) {
      full_prefix += L":";
      full_prefix += std::to_wstring(line_number_);
    }
    full_prefix += L" ";
  } else if (show_line_numbers) {
    full_prefix += std::to_wstring(line_number_);
    full_prefix += L" ";
  }
  full_prefix += level_prefix;

  // Serialize the actual sink writes so concurrent threads can't interleave
  // mid-line on stdout/stderr/file. Each `<<` on std::wcout is its own
  // operation, and the synchronized-with-stdio guarantee is only on
  // single-character flushes - characters from different threads can still
  // be intermixed without a lock here. Held only across the flush so the
  // earlier `<<` chains into stream_ stay lock-free.
  static std::mutex sink_mutex;
  {
    std::lock_guard<std::mutex> lock(sink_mutex);
    if (log_to_console_) {
      // Levels higher than INFO go to stderr
      if (level_ > LOG_INFO) {
        std::wcerr << full_prefix << stream_.str() << std::endl;
      } else {
        std::wcout << full_prefix << stream_.str() << std::endl;
      }
    }
    if (log_to_file_) {
      AppendTextToFile(full_prefix + stream_.str());
    }
  }
  // FATAL always crashes regardless of log destination, and after any file
  // write so the last message is flushed before the debugger break. Done
  // outside the lock so KillApp can't deadlock if it ends up logging.
  if (level_ == LOG_FATAL) {
    OutputDebugStringW(stream_.str().c_str());
    KillApp();
  }
}

logging::LogMessage& logging::LogMessage::operator<<(char value) {
  stream_ << static_cast<wchar_t>(value);
  return *this;
}

logging::LogMessage& logging::LogMessage::operator<<(const char* value) {
  if (value) {
    while (*value) {
      stream_ << static_cast<wchar_t>(*value);
      ++value;
    }
  }
  return *this;
}

logging::LogMessage& logging::LogMessage::operator<<(wchar_t value) {
  stream_ << value;
  return *this;
}

logging::LogMessage& logging::LogMessage::operator<<(const wchar_t* value) {
  if (value) {
    stream_ << value;
  }
  return *this;
}

logging::LogMessage& logging::LogMessage::operator<<(const std::string& value) {
  for (char c : value) {
    stream_ << static_cast<wchar_t>(c);
  }
  return *this;
}

logging::LogMessage& logging::LogMessage::operator<<(const std::wstring& value) {
  stream_ << value;
  return *this;
}

logging::LogMessage& logging::LogMessage::operator<<(int value) {
  stream_ << value;
  return *this;
}

logging::LogMessage& logging::LogMessage::operator<<(unsigned int value) {
  stream_ << value;
  return *this;
}

logging::LogMessage& logging::LogMessage::operator<<(long value) {
  stream_ << value;
  return *this;
}

logging::LogMessage& logging::LogMessage::operator<<(long long value) {
  stream_ << value;
  return *this;
}

logging::LogMessage& logging::LogMessage::operator<<(unsigned long value) {
  stream_ << value;
  return *this;
}

logging::LogMessage& logging::LogMessage::operator<<(unsigned long long value) {
  stream_ << value;
  return *this;
}

logging::LogMessage& logging::LogMessage::operator<<(float value) {
  stream_ << std::setprecision(std::numeric_limits<float>::digits10) << value;
  return *this;
}

logging::LogMessage& logging::LogMessage::operator<<(double value) {
  stream_ << std::setprecision(std::numeric_limits<double>::digits10) << value;
  return *this;
}

logging::LogMessage& logging::LogMessage::operator<<(long double value) {
  stream_ << std::setprecision(std::numeric_limits<long double>::digits10) << value;
  return *this;
}

logging::LogMessage& logging::LogMessage::operator<<(HWND value) {
  stream_ << std::fixed << std::showbase << std::hex << reinterpret_cast<unsigned long long>(value)
          << std::dec << std::noshowbase << std::defaultfloat;
  return *this;
}

// TODO: Add DCHECK/DLOG with more Chromium like
bool logging::LogMessage::IsDCheck() {
  return dcheck_log_;
}

void logging::SetIsDCheck(bool set_is_dcheck) {
  dcheck_log_ = set_is_dcheck;
}

bool logging::InitLogging(HINSTANCE hInstance, LogInitSettings InitSettings) {
  CHECK(!logging_initialized);
  bool success                    = false;
  LogDest log_sink                = InitSettings.log_sink;
  const std::wstring logfile_name = InitSettings.logfile_name;
  const std::wstring app_name     = InitSettings.app_name;
  show_func_sigs                  = InitSettings.show_func_sigs;
  show_line_numbers               = InitSettings.show_line_numbers;
  show_time                       = InitSettings.show_time;
  full_prefix_level               = InitSettings.full_prefix_level;
  if (!hInstance || log_sink >= MAX_LOG_DEST) {
    logging_initialized = false;
    return false;
  }
  kProgName                      = app_name;
  kLogFileName                   = logfile_name;
  const bool is_console_attached = GetIsConsoleAttached();
  const std::wstring logfile     = GetCurrentRelDir() + kLogFileName;
  switch (log_sink) {
    case LOG_NONE:
      logging_initialized = false;
      return true;
      break;
    case MAX_LOG_DEST:
      success = false;
      break;
    case LOG_TO_FILE:
      success = OpenFileForWriting(logfile);
      break;
    case LOG_TO_ALL: {
      if (!is_console_attached) {
        const int attach_code = AttachConsoleImpl();
        if (attach_code == 0 || attach_code == 1) {
          success = OpenFileForWriting(logfile);
        } else {
          success = false;
        }
      } else {
        success = OpenFileForWriting(logfile);
      }
    } break;
    case LOG_TO_STDERR: {
      if (!is_console_attached) {
        const int attach_code = AttachConsoleImpl();
        success               = attach_code == 0 || attach_code == 1;
      } else {
        success = true;
      }
    } break;
    default:
      NOTREACHED();
      break;
  }
  logging_initialized = success;
  return success;
}

bool logging::DeInitLogging(HINSTANCE hInstance) {
  if (!hInstance) {
    return false;
  }
  const bool file_is_open        = IsFileOpen();
  const bool is_console_attached = GetIsConsoleAttached();
  const bool closed_files        = file_is_open ? CloseFileHandle() : true;
  CHECK(closed_files);
  bool detached_everything = false;
  if (!is_console_attached) {
    detached_everything = closed_files;
  } else {
    detached_everything = closed_files && DetachConsoleImpl();
  }
  logging_initialized = !detached_everything;
  return detached_everything;
}

// Tests the various operator overloads for basic types, and macros.
void logging::TestLogging() {
  std::cout << "[INFO] Testing logging with different types " << std::endl;
  LOG(INFO) << "Info1: ostream " << L"Info2 wostream ";
  static constexpr char info3[]    = "Info3 char ";
  static constexpr wchar_t info4[] = L"Info4 wchar_t ";
  static const std::string info5   = "Info5 string ";
  static const std::wstring info6  = L"Info6 wstring ";
  std::ostringstream info7;
  std::wostringstream info8;
  info7 << "Info7 ostringstream ";
  info8 << L"Info8 wostringstream ";
  static constexpr float testFl               = 3.141592f;
  static constexpr unsigned long long testULL = std::numeric_limits<unsigned long long>::max();
  static constexpr long double testDb         = 3.141592653589793238462643383279L;
  static constexpr DWORD testDword            = static_cast<DWORD>(0x0003);
  LOG(INFO) << info3 << info4;
  LOG(INFO) << info5 << info6;
  LOG(INFO) << info7.str() << info8.str();
  LOG(WARN) << "Test DWORD (hex): " << logging::Hex(testDword);
  LOG(WARN) << "Test DWORD (decimal): " << testDword;
  LOG(WARN) << "Test float: " << testFl;
  LOG(DEBUG) << "Test unsigned long long: " << testULL;
  LOG(DEBUG) << "Test long double: " << testDb;
  LOG(ERROR) << "Test Error";
  LOG(ERROR) << L"Test Error " << logging::Hex(GetLastError());
  DLOG() << L"DLOG() Test. CW_USEDEFAULT: " << CW_USEDEFAULT;
  if (test_fatal) {
    LOG(FATAL) << L"Testing wide character FATAL logging";
  }
  CLOG(INFO) << L"CLOG() Test. YOU SHOULD NOT SEE THIS IN LOG FILE! ";
  FLOG(INFO) << L"FLOG() Test. YOU SHOULD NOT SEE THIS IN CONSOLE! ";
  // LOG(INFO) << L"IDR_MAINFRAME" << IDR_MAINFRAME;
  // LOG(INFO) << L"IDI_MAINFRAME" << IDI_MAINFRAME;
  // LOG(INFO) << L"IDC_MAINFRAME" << IDC_MAINFRAME;
  // LOG(INFO) << L"IDM_MAINFRAME" << IDM_MAINFRAME;
  // LOG(INFO) << L"_APS_NO_MFC" << _APS_NO_MFC;
  // LOG(INFO) << L"_APS_NEXT_RESOURCE_VALUE" << _APS_NEXT_RESOURCE_VALUE;
  // LOG(INFO) << L"_APS_NEXT_COMMAND_VALUE" << _APS_NEXT_COMMAND_VALUE;
  // LOG(INFO) << L"_APS_NEXT_CONTROL_VALUE" << _APS_NEXT_CONTROL_VALUE;
  // LOG(INFO) << L"_APS_NEXT_SYMED_VALUE" << _APS_NEXT_SYMED_VALUE;
}
