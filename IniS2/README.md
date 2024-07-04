# A PoC for the old GetPrivateProfileString

It's demonstrated that it can handle all sorts of local paths but not WSL paths (presumably no network paths)

```text
Reading with CRLF on Windows
Found user "armstrong" in D:\wsl.conf



Reading with LF-only on Windows
Found user "armstrong" in D:\wsl.conf



Reading with CRLF on Windows with UNC Paths
Found user "armstrong" in \\localhost\d$\wsl.conf



Reading with LF-only on Windows with UNC Paths
Found user "armstrong" in \\localhost\d$\wsl.conf



Reading with CRLF on Win32 File Namespaces
Found user "armstrong" in \\?\d:\wsl.conf



Reading with LF-only on Win32 File Namespaces
Found user "armstrong" in \\?\d:\wsl.conf



Reading with CRLF on WSL
Error 1 when attempting to read \\wsl.localhost\UbuntuWork3\home\cnihelton\wsl.conf whose contents are:
# CRLF on WSL
[boot]
systemd=true
[user]
default=armstrong



Reading with LF-only on WSL
Error 1 when attempting to read \\wsl.localhost\UbuntuWork3\home\cnihelton\wsl.conf whose contents are:
# LF-only on WSL
[boot]
systemd=true
[user]
default=armstrong
```