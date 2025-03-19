#include "windows.h"
#include "stdbool.h"
#include "stdio.h"

typedef struct {
    byte isHMDSensorAttached;
    byte isHMDAttached;
    byte isLatencyTesterAttached;
} MessageList;

typedef struct {
    double qw;
    double qx;
    double qy;
    double qz;
} TUNNEL_DATA;

bool (*OVR_Initialize) ();
bool (*OVR_Destroy) ();
bool (*OVR_GetSensorOrientation) (int sensorID, float* w, float* x, float* y, float* z);
bool (*OVR_Update) (MessageList* messageList);
int (*OVR_GetSensorCount) ();
bool (*OVR_IsSensorPresent) (int sensorID);
void (*OVR_ProcessLatencyInputs) ();
bool (*OVR_ResetSensorOrientation) (int sensorID);

int main() {
    HANDLE PIPE = CreateFileA(
        "\\\\.\\pipe\\OculusPluginBinding",
        GENERIC_ALL,
        FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (PIPE == INVALID_HANDLE_VALUE && false) {
        char* buffer = NULL;
        FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            GetLastError(),
            NULL,
            (char*)(&buffer),
            0,
            NULL
        );

        MessageBoxA(NULL, buffer, "Unable to connnect named pipe", MB_OK | MB_ICONERROR);
        LocalFree(buffer);

        return 0;
    }

    HMODULE OculusPlugin = LoadLibraryA("OculusPlugin.dll");
    if (OculusPlugin) {
        OVR_Initialize = (void(*)())GetProcAddress(OculusPlugin, "OVR_Initialize");
        OVR_Destroy = (void(*)())GetProcAddress(OculusPlugin, "OVR_Destroy");
        OVR_GetSensorOrientation = (void(*)())GetProcAddress(OculusPlugin, "OVR_GetSensorOrientation");
        OVR_Update = (void(*)())GetProcAddress(OculusPlugin, "OVR_Update");
        OVR_GetSensorCount = (void(*)())GetProcAddress(OculusPlugin, "OVR_GetSensorCount");
        OVR_IsSensorPresent = (void(*)())GetProcAddress(OculusPlugin, "OVR_IsSensorPresent");
        OVR_ProcessLatencyInputs = (void(*)())GetProcAddress(OculusPlugin, "OVR_ProcessLatencyInputs");
        OVR_ResetSensorOrientation = (void(*)())GetProcAddress(OculusPlugin, "OVR_ResetSensorOrientation");
    } else {
        char* buffer = NULL;
        FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            GetLastError(),
            NULL,
            (char*)(&buffer),
            0,
            NULL
        );

        MessageBoxA(NULL, buffer, "failed to load OculusPlugin.dll", MB_OK | MB_ICONERROR);
        LocalFree(buffer);
        return;
    }

    OVR_Initialize();
    OVR_ResetSensorOrientation(0);
    
    MessageList messageList = {0};
    while (true) {
        printf("%i\n", OVR_Update(&messageList));

        TUNNEL_DATA tunnel_data = {0};
        OVR_GetSensorOrientation(0, &tunnel_data.qw, &tunnel_data.qx, &tunnel_data.qy, &tunnel_data.qz);

        printf("%f %f %f %f\n", tunnel_data.qw, tunnel_data.qx, tunnel_data.qy, tunnel_data.qz);

        OVR_ProcessLatencyInputs();

        WriteFile(
            PIPE,
            &tunnel_data,
            sizeof(TUNNEL_DATA),
            NULL,
            NULL
        );
    }

    OVR_Destroy();
	return 0;
}