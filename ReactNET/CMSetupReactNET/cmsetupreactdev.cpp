#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <ndisguid.h>
#include <stdio.h>
#include <tchar.h>

#pragma comment(lib, "cfgmgr32.lib")
#pragma comment(lib, "Setupapi.lib")

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
            // A network device instance has been enumerated
            printf("A new network device instance was enumerated.\n");

            // Retrieve the device instance ID
            TCHAR deviceInstanceId[MAX_DEVICE_ID_LEN];
            _tcscpy_s(deviceInstanceId, EventData->u.DeviceInterface.SymbolicLink);

            // Get device information set for the device instance
            HDEVINFO hDevInfo = SetupDiGetClassDevs(NULL, deviceInstanceId, NULL, DIGCF_PRESENT | DIGCF_PROFILE);
            if (hDevInfo == INVALID_HANDLE_VALUE)
            {
                printf("Failed to get device information set.\n");
                return ERROR_SUCCESS;
            }

            SP_DEVINFO_DATA devInfoData;
            devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
            if (!SetupDiOpenDeviceInfo(hDevInfo, deviceInstanceId, NULL, 0, &devInfoData))
            {
                printf("Failed to open device information.\n");
                SetupDiDestroyDeviceInfoList(hDevInfo);
                return ERROR_SUCCESS;
            }

            // Retrieve the friendly name directly from the device's registry properties
            TCHAR connectionName[256] = { 0 };
            DWORD bufferSize = sizeof(connectionName);
            DWORD type;

            if (SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_FRIENDLYNAME, &type, (PBYTE)connectionName, bufferSize, NULL))
            {
                _tprintf(TEXT("Connection Name: %s\n"), connectionName);
            }
            else
            {
                _tprintf(TEXT("Failed to retrieve the connection name.\n"));
            }

            SetupDiDestroyDeviceInfoList(hDevInfo);
        }
    }

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
        printf("Failed to register notification. Error: %u\n", cr);
        return 1;
    }

    printf("Notification registered. Waiting for events...\n");

    // Run a message loop or wait indefinitely for the callback to be triggered
    // In a real application, you would have a more robust mechanism to keep the application running
    Sleep(INFINITE);

    CM_Unregister_Notification(hNotification);

    return 0;
}
