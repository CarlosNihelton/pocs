//
//    Copyright (C) Microsoft.  All rights reserved.
// Licensed under the terms described in the LICENSE file in the root of this project.
//

#pragma once
#include <Windows.h>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <wslapi.h>
#include <cstddef>

// This error definition is present in the Spring Creators Update SDK.
#ifndef ERROR_LINUX_SUBSYSTEM_NOT_PRESENT
#define ERROR_LINUX_SUBSYSTEM_NOT_PRESENT 414L
#endif // !ERROR_LINUX_SUBSYSTEM_NOT_PRESENT

namespace wsl {

  enum class ConsoleSpec { AttachToThis, Pipe, Null };

  class PipedConsole {

  public:
    PipedConsole(HANDLE err, HANDLE in, HANDLE out) : stdErr_{err}, stdIn_{in}, stdOut_{out} {}

    ~PipedConsole() {
      CloseHandle(stdErr_);
      CloseHandle(stdIn_);
      CloseHandle(stdOut_);
    }

  private:
    HANDLE stdErr_ = nullptr;
    HANDLE stdIn_  = nullptr;
    HANDLE stdOut_ = nullptr;

    friend class Process;
  };

  class Process {
  public:
    std::uint32_t wait(std::uint32_t milisseconds = INFINITE);
    std::int32_t  pid() const { return pid_; }

    bool readStdOut(std::span<std::byte> destination);
    bool readStdErr(std::span<std::byte> destination);
    bool writeStdIn(std::span<std::byte const> source);

  private:
    HANDLE                      processHandle_ = nullptr;
    std::int32_t                pid_           = -1;
    std::optional<PipedConsole> console;

    friend class Distro;
  };

  typedef BOOL(STDAPICALLTYPE* WSL_IS_DISTRIBUTION_REGISTERED)(PCWSTR);
  typedef HRESULT(STDAPICALLTYPE* WSL_REGISTER_DISTRIBUTION)(PCWSTR, PCWSTR);
  typedef HRESULT(STDAPICALLTYPE* WSL_CONFIGURE_DISTRIBUTION)(PCWSTR, ULONG, WSL_DISTRIBUTION_FLAGS);
  typedef HRESULT(STDAPICALLTYPE* WSL_GET_DISTRIBUTION_CONFIGURATION)(PCWSTR, ULONG*, ULONG*, WSL_DISTRIBUTION_FLAGS*,
                                                                      PSTR**, ULONG*);
  typedef HRESULT(STDAPICALLTYPE* WSL_LAUNCH_INTERACTIVE)(PCWSTR, PCWSTR, BOOL, DWORD*);
  typedef HRESULT(STDAPICALLTYPE* WSL_LAUNCH)(PCWSTR, PCWSTR, BOOL, HANDLE, HANDLE, HANDLE, HANDLE*);

  class WslApi;
  class DistroApi;

  class Provider {
    HMODULE                            _wslApiDll                    = nullptr;
    WSL_IS_DISTRIBUTION_REGISTERED     _isDistributionRegistered     = nullptr;
    WSL_REGISTER_DISTRIBUTION          _registerDistribution         = nullptr;
    WSL_CONFIGURE_DISTRIBUTION         _configureDistribution        = nullptr;
    WSL_GET_DISTRIBUTION_CONFIGURATION _getDistributionConfiguration = nullptr;
    WSL_LAUNCH_INTERACTIVE             _launchInteractive            = nullptr;
    WSL_LAUNCH                         _launch                       = nullptr;

  public:
    Provider();

    WslApi    GetWslApi() const;
    DistroApi GetDistroApi() const;

    bool IsEnabled() const;

    ~Provider() noexcept;
  };

  class WslApi {
    WSL_REGISTER_DISTRIBUTION registerDistribution = nullptr;
    explicit WslApi(WSL_REGISTER_DISTRIBUTION fn) : registerDistribution{fn} {}
    friend class Provider;

  public:
    HRESULT Register(std::wstring_view name, std::filesystem::path const& tarball) const;
  };

  class DistroApi {
    WSL_IS_DISTRIBUTION_REGISTERED     isDistributionRegistered     = nullptr;
    WSL_CONFIGURE_DISTRIBUTION         configureDistribution        = nullptr;
    WSL_GET_DISTRIBUTION_CONFIGURATION getDistributionConfiguration = nullptr;
    WSL_LAUNCH_INTERACTIVE             launchInteractive            = nullptr;
    WSL_LAUNCH                         launch                       = nullptr;

    DistroApi(WSL_IS_DISTRIBUTION_REGISTERED     isDistributionRegistered,
              WSL_CONFIGURE_DISTRIBUTION         configureDistribution,
              WSL_GET_DISTRIBUTION_CONFIGURATION getDistributionConfiguration,
              WSL_LAUNCH_INTERACTIVE launchInteractive, WSL_LAUNCH launch) :
        isDistributionRegistered{isDistributionRegistered},
        configureDistribution{configureDistribution}, getDistributionConfiguration{getDistributionConfiguration},
        launchInteractive{launchInteractive}, launch{launch} {}

    friend class Provider;
    friend class Distro;
  };

  class Distro {
  public:
    explicit Distro(std::wstring distributionName);
    Distro(std::wstring distributionName, DistroApi api);

    bool IsRegistered() const;

    HRESULT SetDefaultUid(std::uint64_t uid);

    HRESULT SetFlags(WSL_DISTRIBUTION_FLAGS wslDistributionFlags);

    struct Configuration {
      short                                        version    = 0;
      std::uint64_t                                defaultUid = 0;
      WSL_DISTRIBUTION_FLAGS                       flags;
      std::unordered_map<std::string, std::string> environment;
    };

    Configuration GetConfiguration();

    std::pair<HRESULT, std::int32_t> Run(std::wstring_view command, bool useCurrentWorkingDirectory);

    Process Start(std::wstring cmd, std::vector<std::wstring> args, bool useCurrentWorkingDirectory, ConsoleSpec console);

  private:
    std::wstring distributionName_;
    DistroApi    api_;
  };
} // namespace wsl
