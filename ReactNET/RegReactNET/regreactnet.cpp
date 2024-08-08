#include <windows.h>
#include <stdio.h>
#include <string>
#include <tchar.h>

ULONG GetConnectionNameFromRegistry(std::wstring contents) {
	// Open the network registry key
	HKEY hKey;


	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
	{
		wprintf(L"Failed to open the network registry key.\n");
		return ERROR_SUCCESS;
	}

	// Enumerate the subkeys to find the matching device instance ID
	DWORD index = 0;
	TCHAR subKeyName[MAX_PATH];
	DWORD subKeyNameSize = MAX_PATH;

	while (RegEnumKeyEx(hKey, index, subKeyName, &subKeyNameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
	{
		HKEY hSubKey;
		if (RegOpenKeyEx(hKey, subKeyName, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
		{
			// Open the "Connection" subkey
			HKEY hConnectionKey;
			if (RegOpenKeyEx(hSubKey, L"Connection", 0, KEY_READ, &hConnectionKey) == ERROR_SUCCESS)
			{
				// Read the "PnpInstanceID" value to check if it matches the device instance ID
				TCHAR name[256] = { 0 };
				DWORD nameSize = sizeof(name);
				DWORD type;

				if (RegQueryValueEx(hConnectionKey, L"Name", NULL, &type, (LPBYTE)name, &nameSize) == ERROR_SUCCESS)
				{
					if (std::wstring_view{ name }.contains(contents))
					{
						wprintf(L"Connection Name: %s\n", name);
					}
					else
					{
						wprintf(L"Failed to retrieve the connection name.\n");
					}

				}
				RegCloseKey(hConnectionKey);
			}
			RegCloseKey(hSubKey);
		}
		subKeyNameSize = MAX_PATH;
		index++;
	}

	RegCloseKey(hKey);

	return ERROR_SUCCESS;
}

void MonitorRegistryKey(HKEY hKey)
{
	DWORD dwFilter = REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_ATTRIBUTES | REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_SECURITY;

	while (TRUE)
	{
		if (RegNotifyChangeKeyValue(hKey, TRUE, dwFilter, NULL, FALSE) == ERROR_SUCCESS)
		{
			printf("Registry key changed. Checking for friendly name...\n");
			GetConnectionNameFromRegistry(L"(WSL");
		}
		else
		{
			printf("Failed to set registry change notification.\n");
			break;
		}
	}
}

int main()
{
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		TEXT("SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}"),
		0,
		KEY_READ | KEY_NOTIFY,
		&hKey) == ERROR_SUCCESS)
	{
		printf("Monitoring registry key for changes...\n");
		MonitorRegistryKey(hKey);
		RegCloseKey(hKey);
	}
	else
	{
		printf("Failed to open registry key.\n");
	}

	return 0;
}
