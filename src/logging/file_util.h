#ifndef MINI_LOGGER_FILE_UTIL_H_
#define MINI_LOGGER_FILE_UTIL_H_

#include "console.h"
#include "logging_base.h"

namespace logging {

  extern HANDLE g_log_file;

  extern volatile bool file_open;

  extern std::wstring kProgName;
  extern std::wstring kLogFileName;

  // Where to output logging to
  enum LogDest {
    LOG_NONE      = 0,
    LOG_TO_FILE   = 1,
    LOG_TO_STDERR = 2,
    LOG_TO_ALL    = 3,
    MAX_LOG_DEST  = 4
  };

  // Convert narrow string to wide string (ASCII only, suitable for __func__, __DATE__, etc.)
  inline const std::wstring ToWide(const char* s) {
    if (!s) {
      return L"";
    }
    std::wstring result;
    while (*s) {
      result += static_cast<wchar_t>(*s++);
    }
    return result;
  }

  // wchar_t override for ease of use
  inline const std::wstring ToWide(const wchar_t* s) {
    if (!s) {
      return L"";
    }
    std::wstring result;
    while (*s) {
      result += *s++;
    }
    return result;
  }

  // Retrieves the current dir of the .exe calling this process
  const std::wstring GetCurrentRelDir();

  // Gets where to put log dir if using current directory fails (i.e. in Admin owned dir)
  // Usually %LOCALAPPDATA%
  const std::wstring GetAppDataDir();

  // Writes a UTF16 BOM so Notepad can handle Unicode
  bool WriteUTF16BOM(HANDLE hFile);

  // Whether the log file should be cleared before writing
  const bool ShouldTruncateLogFile();

  // Creates new or opens existing logfile, and opens it for writing without a process lock
  bool OpenFileForWriting(const std::wstring& logfile_path);

  // Same as above, but if permissions fail, fall back to GetAppDataDir
  bool OpenFileForWritingAlt(const std::wstring& alt_logfile_path,
                             bool should_truncate,
                             bool& out_write_bom);

  // Closes file handles safely, and sets g_log_file back to INVALID_HANDLE_VALUE
  bool CloseFileHandle();

  // Appends a line of text to the end of a file.
  bool AppendTextToFile(const std::wstring& log_line);

  // Clears the logfile.
  bool ClearFileContents();

  // Gets whether file is currently open
  bool IsFileOpen();

  // Returns the full path of the currently open log file, or an empty string
  // if file logging is not active.
  const std::wstring GetLogFilePath();

} // namespace logging

#endif // MINI_LOGGER_FILE_UTIL_H_
