#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <devpkey.h>
#include <ndisguid.h>
#include <string>
#include <cstdio>
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
			if (IsEqualGUID(EventData->u.DeviceInterface.ClassGuid, GUID_DEVINTERFACE_NET))
			{
				// A network device instance has been enumerated
				printf("A new network device instance was enumerated.\n");

				// Retrieve the device instance ID
				TCHAR deviceInstanceId[MAX_DEVICE_ID_LEN] = { 0 };
				_tcscpy_s(deviceInstanceId, EventData->u.DeviceInterface.SymbolicLink);

				// Get device information set for the device instance
				HDEVINFO hDevInfo = SetupDiGetClassDevs(&(EventData->u.DeviceInterface.ClassGuid), NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT | DIGCF_PROFILE);
				if (hDevInfo == INVALID_HANDLE_VALUE)
				{
					printf("Failed to get device information set.\n");
					return ERROR_SUCCESS;
				}

				SP_DEVINFO_DATA devInfoData;
				devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
				// Funny enough that matches the adapter name as seen in the "Device Manager", not the connection name which is what we needed to identify WSL.
				for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); i++)
				{
					TCHAR friendlyName[256];
					DWORD size = 0;

					// Get the device description
					if (SetupDiGetDeviceRegistryProperty(
						hDevInfo,
						&devInfoData,
						SPDRP_FRIENDLYNAME,
						NULL,
						(PBYTE)friendlyName,
						sizeof(friendlyName),
						&size))
					{
						printf("Device matches the pattern: %ws\n", friendlyName);
					}
				}

				SetupDiDestroyDeviceInfoList(hDevInfo);
			}
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
