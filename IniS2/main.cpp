#include <Windows.h>
#include <algorithm>
#include <iostream>
#include <filesystem>;
#include <fstream>
#include <print>

struct FileModel {
	std::string_view path, contents, comment;

	void writeFile() {
		std::ofstream file{ path.data(), std::ios::binary };
		std::println(file, "# {}", comment);
		std::copy(std::begin(contents), std::end(contents), std::ostreambuf_iterator<char>(file));
	}

	void parseFile() {
		std::println("Reading with {}", comment);

		char uname[32] = { '\0' };
		if (auto len = GetPrivateProfileStringA("user", "default", nullptr, uname, 31, path.data()); len > 0) {
			std::println(R"(Found user "{}" in {})", uname, path);
		}
		else {
			std::println("Error {} when attempting to read {} whose contents are:", GetLastError(), path);
			std::ifstream file{ path.data(), std::ios::binary };
			std::copy(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>{}, std::ostreambuf_iterator<char>(std::cout));
		}
		std::puts("\n\n");
	}
};

int main() {
	// Adjust the paths to your hearts content:
	static constexpr auto winPath = "D:\\wsl.conf";
	static constexpr auto linuxPath = "\\\\wsl.localhost\\UbuntuWork3\\home\\cnihelton\\wsl.conf";

	static constexpr std::string_view winContents = "[boot]\r\nsystemd=true\r\n[user]\r\ndefault=armstrong\r\n";
	std::string linuxContents(winContents.size(), '\0');
	std::copy_if(std::begin(winContents), std::end(winContents), std::back_inserter(linuxContents), [](const char c) {return c != '\r'; });

	static constexpr auto uncLocalPath = "\\\\localhost\\d$\\wsl.conf";
	static constexpr auto win32FileNamespacePath = "\\\\?\\d:\\wsl.conf";

	FileModel winCrLf({ .path = winPath, .contents = winContents, .comment = "CRLF on Windows" });
	winCrLf.writeFile();
	winCrLf.parseFile();

	FileModel winLf({ .path = winPath, .contents = linuxContents, .comment = "LF-only on Windows" });
	winLf.writeFile();
	winLf.parseFile();

	FileModel uncLocal({ .path = uncLocalPath, .contents = winContents, .comment = "CRLF on Windows with UNC Paths" });
	uncLocal.writeFile();
	uncLocal.parseFile();

	FileModel uncLocalLf({ .path = uncLocalPath, .contents = linuxContents, .comment = "LF-only on Windows with UNC Paths" });
	uncLocalLf.writeFile();
	uncLocalLf.parseFile();

	FileModel win32NsCrLf({ .path = win32FileNamespacePath, .contents = winContents, .comment = "CRLF on Win32 File Namespaces" });
	win32NsCrLf.writeFile();
	win32NsCrLf.parseFile();

	FileModel win32NsLf({ .path = win32FileNamespacePath, .contents = linuxContents, .comment = "LF-only on Win32 File Namespaces" });
	win32NsLf.writeFile();
	win32NsLf.parseFile();

	FileModel wslCrLf({ .path = linuxPath, .contents = winContents, .comment = "CRLF on WSL" });
	wslCrLf.writeFile();
	wslCrLf.parseFile();

	FileModel wslLf({ .path = linuxPath, .contents = linuxContents, .comment = "LF-only on WSL" });
	wslLf.writeFile();
	wslLf.parseFile();
	
}
