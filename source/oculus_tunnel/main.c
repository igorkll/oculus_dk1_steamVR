#include "windows.h"
#include "stdbool.h"

void (*OVR_Initialize) ();
void (*OVR_Destroy) ();
void (*OVR_GetSensorOrientation) (double* x, double* y, double* z);

typedef struct {
    double ex;
    double ey;
    double ez;

    double qx;
    double qy;
    double qz;
    double qw;
} TUNNEL_DATA;

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

    if (PIPE == INVALID_HANDLE_VALUE) {
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
    
    while (true) {
        TUNNEL_DATA tunnel_data;
        OVR_GetSensorOrientation(&tunnel_data.ex, &tunnel_data.ey, &tunnel_data.ez);

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