#pragma once

// Makes the calling thread DPI-aware for its lifetime, so screen metrics,
// windows and BitBlt all use physical pixels. The rest of the UI stays
// DPI-unaware and is scaled by Windows. Needs Windows 10 1607+; a no-op before.
class ScopedPhysicalDpi
{
public:
  ScopedPhysicalDpi() : set_context_(NULL), old_context_(NULL)
  {
    HMODULE user32 = ::GetModuleHandle(_T("user32.dll"));
    if (user32)
      set_context_ = (SetContextFn)::GetProcAddress(user32, "SetThreadDpiAwarenessContext");
    if (set_context_)
    {
      old_context_ = set_context_((HANDLE)-4);    // PER_MONITOR_AWARE_V2
      if (!old_context_)
        old_context_ = set_context_((HANDLE)-3);  // PER_MONITOR_AWARE
    }
  }

  ~ScopedPhysicalDpi()
  {
    if (old_context_)
      set_context_(old_context_);
  }

private:
  typedef HANDLE (WINAPI *SetContextFn)(HANDLE);
  SetContextFn set_context_;
  HANDLE old_context_;
};
