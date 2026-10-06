#include "check.h"

#include "logging.h"

// Intentionally execute an invalid opcode to kill the program and signal to debugger
// See http://ref.x86asm.net/coder32.html
void logging::KillApp() {
#ifdef __MINGW32__
  asm("int3\n\t"
      "ud2");
#else
  __asm int 3 // Execute int3 interrupt
      __asm {
    UD2
  } // Execute 0x0F, 0x0B
#endif // __MINGW32__
}

void logging::CheckImpl(const char* func_sig,
                        const int line_num,
                        const char* condition,
                        bool check_flag) {
  if (check_flag) {
    logging::LogMessage(LOG_FATAL, true, true, func_sig, line_num)
        << L"Check failed: " << ToWide(condition);
  }
}

void logging::NotReachedImpl(const char* func_sig, const int line_num) {
  std::wstring msg = ToWide(func_sig) + L"():" + std::to_wstring(line_num) + L" NOTREACHED()";
  std::wcerr << msg << std::endl;
  OutputDebugStringW(msg.c_str());
  KillApp();
}
