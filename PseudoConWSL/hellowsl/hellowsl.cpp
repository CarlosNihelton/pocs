// hellowsl.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Wsl.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <iostream>
#include <string>
#include <vector>
#include <winerror.h>

void printError(std::string_view msg, std::uint32_t code) {
  std::cerr << msg << "0x" << std::setfill('0') << std::setw(2) << std::right << std::hex << code << '\n';
}

//int WINAPI  wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow){
int main() {
  wsl::Distro ubuntu{L"UbuntuWork"};
  if (!ubuntu.IsRegistered()) {
    std::cerr << "Distro not registered\n";
    return __COUNTER__;
  }

  // The simple use case
  auto [hr, exitCode] = ubuntu.Run(L"uname -r", false);
  if (FAILED(hr)) {
    printError("Failed to launch interactively", hr);
    return __COUNTER__;
  }

  if (exitCode != 0) {
    printError("Process failed: ", exitCode);
    return __COUNTER__;
  }

  // The advanced use cases.
  auto                      ping = L"ping";
  std::vector<std::wstring> args{L"-c", L"8", L"8.8.8.8"};
  // Attaching the console - pretty much the same behavior as above:
  {
    wsl::Process p  = ubuntu.Start(ping, args, false, wsl::ConsoleSpec::AttachToThis);
    auto         ex = p.wait();

    std::cout << "Gradual Ping exited: " << ex << '\n';
  }
  // No output - i.e. like > /dev/null
  {
    wsl::Process p  = ubuntu.Start(L"touch", {L"~/hello-invalid-handles"}, false, wsl::ConsoleSpec::Pipe);
    auto         ex = p.wait();

    std::cout << "Silent Ping exited: " << ex << '\n';
  }
  // Piping the console
  {
    wsl::Process p  = ubuntu.Start(ping, args, false, wsl::ConsoleSpec::Pipe);
    auto         ex = p.wait();

    std::cout << "Abrupt Ping exited: " << ex << '\n';

    std::array<std::byte, 1024> buf{};
    if (ex == 0) {
      if (p.readStdOut(buf)) {
        std::cout << std::string_view{reinterpret_cast<char const*>(&buf[0])};
      }
    } else {
      if (p.readStdErr(buf)) {
        std::cerr << std::string_view{reinterpret_cast<char const*>(&buf[0])};
      }
    }
  }
  return 0;
}
