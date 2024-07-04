#pragma once
#include <Windows.h>
#include <string>
#include <type_traits>
#include <utility>

namespace Pro {

  /// Models a process we spawn under a pseudo console.
  class Process {
    std::wstring command;
    HANDLE       me         = nullptr;
    HANDLE       mainThread = nullptr;
    HANDLE       stdInput   = nullptr;
    HANDLE       stdOutputs = nullptr;

  public:
    struct Result {
      HRESULT WaitResult = 0;
      DWORD   ExitCode   = 0;
    };
    /// Waits for process to exit up until `miliseconds`, returning a process result structure containing the result of
    /// the wait operation and the process exit code, if it already terminated.
    Result Wait(DWORD miliseconds);

    /// Blocks the caller while reading the process's stdout/stderr so to prevent it from freezing while writing to the
    /// console. The contents read are simply discarded. This only exits when the console pipe is broken, which is
    /// likely to mean that the child process exited.
    void BlockDrainingOutputs();

    Process() = default;
    Process(Process&& other) noexcept :
        command{std::move(other.command)}, me{std::exchange(other.me, nullptr)},
        mainThread{std::exchange(other.mainThread, nullptr)}, stdInput{std::exchange(other.stdInput, nullptr)},
        stdOutputs{std::exchange(other.stdOutputs, nullptr)} {}

    ~Process();

    friend class PseudoConsole;
  };

  /// Models a Windows Pseudo Console, with which we can have fine grained control over how child console application
  /// processes will appear to the user.
  class PseudoConsole {
    HPCON               hpcon = nullptr;
    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, true};
    HANDLE              hInR = nullptr, hInW = nullptr, hOutR = nullptr, hOutW = nullptr;

  public:
    explicit PseudoConsole(COORD coords);

    HPCON GetHandle() const { return hpcon; }

    PseudoConsole(PseudoConsole&& other) noexcept :
        hpcon{std::exchange(other.hpcon, nullptr)}, hInR{std::exchange(other.hInR, nullptr)},
        hInW{std::exchange(other.hInW, nullptr)}, hOutR{std::exchange(other.hOutR, nullptr)},
        hOutW{std::exchange(other.hOutW, nullptr)} {}

    ~PseudoConsole() noexcept;

    /// Starts a new process by running `command` under this pseudo console.
    Process Start(std::wstring command);
  };
} // namespace Pro
