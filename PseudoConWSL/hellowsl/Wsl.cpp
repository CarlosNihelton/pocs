//
//    Copyright (C) Microsoft.  All rights reserved.
// Licensed under the terms described in the LICENSE file in the root of this project.
//

#include "Wsl.h"
#include <array>
#include <iostream>
#include <libloaderapi.h>
#include <numeric>

namespace wsl {

  Provider::Provider() {
    if (_wslApiDll == nullptr) {
      _wslApiDll = LoadLibraryEx(L"wslapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    }
    if (_wslApiDll != nullptr) {
      _isDistributionRegistered =
          (WSL_IS_DISTRIBUTION_REGISTERED)GetProcAddress(_wslApiDll, "WslIsDistributionRegistered");
      _registerDistribution  = (WSL_REGISTER_DISTRIBUTION)GetProcAddress(_wslApiDll, "WslRegisterDistribution");
      _configureDistribution = (WSL_CONFIGURE_DISTRIBUTION)GetProcAddress(_wslApiDll, "WslConfigureDistribution");
      _getDistributionConfiguration =
          (WSL_GET_DISTRIBUTION_CONFIGURATION)GetProcAddress(_wslApiDll, "WslGetDistributionConfiguration");
      _launchInteractive = (WSL_LAUNCH_INTERACTIVE)GetProcAddress(_wslApiDll, "WslLaunchInteractive");
      _launch            = (WSL_LAUNCH)GetProcAddress(_wslApiDll, "WslLaunch");
    }
  }

  bool Provider::IsEnabled() const {
    return ((_wslApiDll != nullptr) && (_isDistributionRegistered != nullptr) && (_registerDistribution != nullptr) &&
            (_configureDistribution != nullptr) && (_getDistributionConfiguration != nullptr) &&
            (_launchInteractive != nullptr) && (_launch != nullptr));
  }

  Provider ::~Provider() noexcept {
    if (_wslApiDll != nullptr) {
      FreeLibrary(_wslApiDll);
      _wslApiDll = nullptr;
    }
  }

  WslApi Provider::GetWslApi() const { return WslApi{_registerDistribution}; }

  DistroApi Provider::GetDistroApi() const {
    return DistroApi{_isDistributionRegistered, _configureDistribution, _getDistributionConfiguration,
                     _launchInteractive, _launch};
  }

  namespace {
    Provider& wsl() {
      static Provider api{};
      return api;
    }
  } // namespace

  Distro::Distro(std::wstring distributionName, DistroApi api) :
      distributionName_{std::move(distributionName)}, api_{std::move(api)} {}

  Distro::Distro(std::wstring distributionName) : Distro::Distro(std::move(distributionName), wsl().GetDistroApi()) {}

  bool Distro::IsRegistered() const { return api_.isDistributionRegistered(distributionName_.c_str()); }

  HRESULT WslApi::Register(std::wstring_view name, std::filesystem::path const& tarball) const {
    return registerDistribution(name.data(), tarball.native().c_str());
  }

  std::pair<HRESULT, std::int32_t> Distro::Run(std::wstring_view command, bool useCurrentWorkingDirectory) {
    std::pair<HRESULT, std::int32_t> result{};
    DWORD                            exitCode = 0;
    result.first =
        api_.launchInteractive(distributionName_.c_str(), command.data(), useCurrentWorkingDirectory, &exitCode);
    result.second = static_cast<std::int32_t>(exitCode);
    return result;
  }

  Process Distro::Start(std::wstring cmd, std::vector<std::wstring> args, bool useCurrentWorkingDirectory,
                        ConsoleSpec console) {
    HANDLE              p = nullptr, in = nullptr, out = nullptr, err = nullptr;
    HANDLE              launchIn = nullptr, launchOut = nullptr, launchErr = nullptr;
    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, true};
    switch (console) {
    case ConsoleSpec::AttachToThis:
      launchErr = GetStdHandle(STD_ERROR_HANDLE);
      launchIn  = GetStdHandle(STD_INPUT_HANDLE);
      launchOut = GetStdHandle(STD_OUTPUT_HANDLE);
      break;
    case ConsoleSpec::Pipe:

      if (0 == CreatePipe(&launchIn, &in, &sa, 0)) {
        throw GetLastError();
      }
      if (0 == CreatePipe(&out, &launchOut, &sa, 0)) {
        throw GetLastError();
      }
      if (0 == CreatePipe(&err, &launchErr, &sa, 0)) {
        throw GetLastError();
      }
      break;
    case ConsoleSpec::Null:
      launchErr = INVALID_HANDLE_VALUE;
      launchIn  = INVALID_HANDLE_VALUE;
      launchOut = INVALID_HANDLE_VALUE;
      break;
    }

    std::size_t joinedSize =
        1 + std::accumulate(std::begin(args), std::end(args), cmd.size(),
                            [](std::size_t a, std::wstring const& b) { return b.size() + 1 + a; });
    std::wstring cmdWithArgs{};
    cmdWithArgs.reserve(joinedSize);
    cmdWithArgs = std::accumulate(std::begin(args), std::end(args), cmd,
                                  [](std::wstring const& a, std::wstring const& b) { return a + L' ' + b; });

    HRESULT hr = api_.launch(distributionName_.c_str(), cmdWithArgs.c_str(), useCurrentWorkingDirectory, launchIn,
                             launchOut, launchErr, &p);
    if (FAILED(hr)) {
      throw hr;
    }

    if (p == nullptr) {
      throw ERROR_INVALID_HANDLE;
    }
    Process pr{};
    pr.processHandle_ = p;
    pr.pid_           = GetProcessId(p);

    if (console == ConsoleSpec::Pipe) {
      pr.console.emplace(err, in, out);
      CloseHandle(launchErr);
      CloseHandle(launchIn);
      CloseHandle(launchOut);
    }

    return pr;
  }

  std::uint32_t Process::wait(std::uint32_t milisseconds) {
    if (auto wait_ = WaitForSingleObject(processHandle_, milisseconds); wait_ != WAIT_OBJECT_0) {
      if (wait_ == -1) {
        return GetLastError();
      }
      return wait_;
    }
    DWORD exitCode = 0;
    auto  g        = GetExitCodeProcess(processHandle_, &exitCode);
    return exitCode;
  }

  bool readOut(std::span<std::byte> destination, HANDLE source) {
    DWORD bytesRead;
    return 0 != ReadFile(source, &destination[0], (destination.size() - 1), &bytesRead, nullptr) && bytesRead != 0;
  }

  bool Process::readStdErr(std::span<std::byte> destination) {
    if (!console.has_value()) {
      return false;
    }
    return readOut(destination, console.value().stdErr_);
  }
  bool Process::readStdOut(std::span<std::byte> destination) {
    if (!console.has_value()) {
      return false;
    }
    return readOut(destination, console.value().stdOut_);
  }

} // namespace wsl
