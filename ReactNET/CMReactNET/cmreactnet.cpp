#include <windows.h>
#include <cfgmgr32.h>
#include <ndisguid.h>
#include <cstdio>
#include <string>
#include <format>

#pragma comment(lib, "cfgmgr32.lib")

ULONG GetConnectionNameFromRegistry(std::wstring id);

ULONG CALLBACK DeviceChangeCallback(
    HCMNOTIFICATION hNotify,
    PVOID Context,
    CM_NOTIFY_ACTION Action,
    PCM_NOTIFY_EVENT_DATA EventData,
    DWORD EventDataSize
)
{
    if (Action == CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL)
    {
        if (EventData->FilterType == CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE)
        {
            // A network device has been created
            wprintf(L"A new network device was created.\n");
            std::wstring deviceInstanceId{ EventData->u.DeviceInterface.SymbolicLink};
            // Get the connection name from the registry
            return GetConnectionNameFromRegistry(deviceInstanceId);
        }
    }

    return ERROR_SUCCESS;
}

ULONG GetConnectionNameFromRegistry(std::wstring id) {
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
                TCHAR pnpInstanceId[MAX_DEVICE_ID_LEN] = { 0 };
                DWORD pnpInstanceIdSize = sizeof(pnpInstanceId);
                DWORD type;

                if (RegQueryValueEx(hConnectionKey, L"PnpInstanceID", NULL, &type, (LPBYTE)pnpInstanceId, &pnpInstanceIdSize) == ERROR_SUCCESS)
                {
                    if (id == pnpInstanceId)
                    {
                        // Read the "Name" value to get the connection name
                        TCHAR connectionName[256];
                        DWORD connectionNameSize = sizeof(connectionName);

                        if (RegQueryValueEx(hConnectionKey, L"Name", NULL, &type, (LPBYTE)connectionName, &connectionNameSize) == ERROR_SUCCESS)
                        {
                            wprintf(L"Connection Name: %s\n", connectionName);
                        }
                        else
                        {
                            wprintf(L"Failed to retrieve the connection name.\n");
                        }
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

int main()
{
    CM_NOTIFY_FILTER cmFilter = { 0 };
    cmFilter.cbSize = sizeof(CM_NOTIFY_FILTER);
    cmFilter.Flags = 0;
    cmFilter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
    cmFilter.u.DeviceInterface.ClassGuid = GUID_DEVINTERFACE_NET;

    HCMNOTIFICATION hNotification;
    CONFIGRET cr = CM_Register_Notification(
        &cmFilter,
        NULL,
        DeviceChangeCallback,
        &hNotification
    );

    if (cr != CR_SUCCESS)
    {
        wprintf(L"Failed to register notification. Error: %u\n", cr);
        return 1;
    }

    wprintf(L"Notification registered. Waiting for events...\n");

    // Run a message loop or wait indefinitely for the callback to be triggered
    // In a real application, you would have a more robust mechanism to keep the application running
    Sleep(INFINITE);

    CM_Unregister_Notification(hNotification);

    return 0;
}
