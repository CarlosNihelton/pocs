#include "PseudoConsole.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace Pro {
  Process::Result Process::Wait(DWORD miliseconds) {
    if (auto Wait = WaitForSingleObject(me, miliseconds); Wait != WAIT_OBJECT_0) {
      return {HRESULT_FROM_WIN32(GetLastError()), static_cast<DWORD>(-1)};
    }
    DWORD exitCode = 0;
    if (GetExitCodeProcess(me, &exitCode) == 0) {
      return {HRESULT_FROM_WIN32(GetLastError()), static_cast<DWORD>(-1)};
    }
    return {S_OK, exitCode};
  }

  void Process::BlockDrainingOutputs() {
    static constexpr DWORD       magic = 256;
    DWORD                        bytesRead;
    std::array<std::byte, magic> buffer{};
    DWORD                        newPipeMode = PIPE_READMODE_BYTE | PIPE_NOWAIT;
    SetNamedPipeHandleState(stdOutputs, &newPipeMode, NULL, NULL);
    for (DWORD exitCode = STILL_ACTIVE; exitCode == STILL_ACTIVE; Sleep(magic)) {
      if (0 != ReadFile(stdOutputs, &buffer[0], (magic - 1), &bytesRead, nullptr)) {
        continue;
      }
      auto last = GetLastError();
      if (last == ERROR_NO_DATA || last == ERROR_PIPE_NOT_CONNECTED || last == ERROR_BROKEN_PIPE) {
        GetExitCodeProcess(me, &exitCode);
      }
    }
  }

  Process::~Process() {
    if (me)
      CloseHandle(me);
    if (mainThread)
      CloseHandle(mainThread);
  }

  PseudoConsole::PseudoConsole(COORD coords) {
    CreatePipe(&hInR, &hInW, &sa, 0);
    CreatePipe(&hOutR, &hOutW, &sa, 0);
    if (auto hr = CreatePseudoConsole(coords, hInR, hOutW, 0, &hpcon); FAILED(hr)) {
      throw HRESULT_FROM_WIN32(GetLastError());
    }
  }

  void deleter(PPROC_THREAD_ATTRIBUTE_LIST p) {
    if (p) {
      DeleteProcThreadAttributeList(p);
      HeapFree(GetProcessHeap(), 0, p);
    }
  }
  using unique_attr_list = std::unique_ptr<std::remove_pointer_t<PPROC_THREAD_ATTRIBUTE_LIST>, decltype(&deleter)>;
  namespace {
    unique_attr_list AttributeList(HPCON con);
  }

  Process PseudoConsole::Start(std::wstring command) {
    unique_attr_list    attrs = AttributeList(GetHandle());
    PROCESS_INFORMATION pi{};

    // Prepare Startup Information structure
    STARTUPINFOEX si{};
    si.StartupInfo.cb         = sizeof(STARTUPINFOEX);
    si.StartupInfo.hStdInput  = hInR;
    si.StartupInfo.hStdOutput = hOutW;
    si.StartupInfo.hStdError  = hOutW;
    si.StartupInfo.dwFlags    = STARTF_USESTDHANDLES;
    si.lpAttributeList        = attrs.get();

    if (!CreateProcessW(NULL, command.data(), NULL, NULL, FALSE, EXTENDED_STARTUPINFO_PRESENT, NULL, NULL,
                        &si.StartupInfo, &pi)) {
      throw HRESULT_FROM_WIN32(GetLastError());
    }
    Process p{};
    p.command    = std::move(command);
    p.me         = pi.hProcess;
    p.mainThread = pi.hThread;
    p.stdInput   = hInW;
    p.stdOutputs = hOutR;

    CloseHandle(hInR);
    hInR = nullptr;
    CloseHandle(hOutW);
    hOutW = nullptr;

    return p;
  }

  PseudoConsole::~PseudoConsole() noexcept {
    if (hpcon)
      ClosePseudoConsole(hpcon);
    if (hInR)
      CloseHandle(hInR);
    if (hInW)
      CloseHandle(hInW);
    if (hOutR)
      CloseHandle(hOutR);
    if (hOutW)
      CloseHandle(hOutW);
  }

  namespace {
    unique_attr_list AttributeList(HPCON con) {
      PPROC_THREAD_ATTRIBUTE_LIST attrs = nullptr;

      size_t bytesRequired = 0;
      InitializeProcThreadAttributeList(NULL, 1, 0, &bytesRequired);
      // Allocate memory to represent the list
      attrs = static_cast<PPROC_THREAD_ATTRIBUTE_LIST>(HeapAlloc(GetProcessHeap(), 0, bytesRequired));
      if (!attrs) {
        throw E_OUTOFMEMORY;
      }

      // Initialize the list memory location
      if (!InitializeProcThreadAttributeList(attrs, 1, 0, &bytesRequired)) {
        throw HRESULT_FROM_WIN32(GetLastError());
      }

      unique_attr_list result{attrs, &deleter};

      if (!UpdateProcThreadAttribute(attrs, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, con, sizeof(con), NULL, NULL)) {
        throw HRESULT_FROM_WIN32(GetLastError());
      }

      return result;
    }
  } // namespace
} // namespace Pro
