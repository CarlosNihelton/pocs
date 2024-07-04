#include <windows.h>
#include "PseudoConsole.hpp"

int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
  // Create the pseudo-console
  Pro::PseudoConsole con{
      {.X = 32, .Y = 32}
  };

  {
    PCWSTR childApplication = L"C:\\Program "
                              L"Files\\WindowsApps\\CanonicalGroupLimited.UbuntuProForWindows_1.0.3.0_x64__"
                              L"hhj52ngek5ykr\\agent\\ubuntu-pro-"
                              L"agent.exe -vvv"; // The agent, deployed with an app execution alias.
    Pro::Process p = con.Start(childApplication);
    p.BlockDrainingOutputs();
    auto [wait, exitCode] = p.Wait(INFINITE);
    //return exitCode;
  }

  {
    // PCWSTR childApplication = L"D:\\poc\\gowsl\\hellowsl\\x64\\Debug\\hello.exe"; // Go CLI application.
     PCWSTR childApplication = L"D:\\poc\\gowsl\\hellowsl\\x64\\Debug\\hellowsl.exe"; // C++ CLI application
    // PCWSTR childApplication = L"C:\\Windows\\System32\\cmd.exe"; // Windows stuff
    Pro::Process p = con.Start(childApplication);
    p.BlockDrainingOutputs();
    auto [wait, exitCode] = p.Wait(INFINITE);
    //return exitCode;
  }
}
