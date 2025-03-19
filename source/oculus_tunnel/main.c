#include "windows.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdint.h"

//#define DEBUG

typedef struct {
    byte isHMDSensorAttached;
    byte isHMDAttached;
    byte isLatencyTesterAttached;
} MessageList;

typedef struct {
    float qx;
    float qy;
    float qz;
    float qw;
} TUNNEL_DATA;

bool (*OVR_Initialize) ();
bool (*OVR_Destroy) ();
bool (*OVR_GetSensorOrientation) (int sensorID, float* w, float* x, float* y, float* z);
bool (*OVR_GetSensorOrientationQ) (int sensorID, void* unknown);
bool (*OVR_GetSensorPredictedOrientation) (int sensorID, float* w, float* x, float* y, float* z);
bool (*OVR_GetSensorPredictedOrientationQ) (int sensorID, void* unknown);
bool (*OVR_Update) (MessageList* messageList);
int (*OVR_GetSensorCount) ();
bool (*OVR_IsSensorPresent) (int sensorID);
void (*OVR_ProcessLatencyInputs) ();
bool (*OVR_ResetSensorOrientation) (int sensorID);
bool (*OVR_SetSensorPredictionTime) (int sensorID, float predictionTime);
bool (*OVR_EnableMagYawCorrection) (int sensorID, bool enable);
void (*OVR_BeginMagAutoCalibraton) ();

int main() {
#ifndef DEBUG
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
#endif

    HMODULE OculusPlugin = LoadLibraryA("OculusPlugin.dll");
    if (OculusPlugin) {
        OVR_Initialize = (void(*)())GetProcAddress(OculusPlugin, "OVR_Initialize");
        OVR_Destroy = (void(*)())GetProcAddress(OculusPlugin, "OVR_Destroy");
        OVR_GetSensorOrientation = (void(*)())GetProcAddress(OculusPlugin, "OVR_GetSensorOrientation");
        OVR_GetSensorOrientationQ = (void(*)())GetProcAddress(OculusPlugin, "OVR_GetSensorOrientationQ");
        OVR_GetSensorPredictedOrientationQ = (void(*)())GetProcAddress(OculusPlugin, "OVR_GetSensorPredictedOrientationQ");
        OVR_GetSensorPredictedOrientation = (void(*)())GetProcAddress(OculusPlugin, "OVR_GetSensorPredictedOrientation");
        OVR_Update = (void(*)())GetProcAddress(OculusPlugin, "OVR_Update");
        OVR_GetSensorCount = (void(*)())GetProcAddress(OculusPlugin, "OVR_GetSensorCount");
        OVR_IsSensorPresent = (void(*)())GetProcAddress(OculusPlugin, "OVR_IsSensorPresent");
        OVR_ProcessLatencyInputs = (void(*)())GetProcAddress(OculusPlugin, "OVR_ProcessLatencyInputs");
        OVR_ResetSensorOrientation = (void(*)())GetProcAddress(OculusPlugin, "OVR_ResetSensorOrientation");
        OVR_SetSensorPredictionTime = (void(*)())GetProcAddress(OculusPlugin, "OVR_SetSensorPredictionTime");
        OVR_EnableMagYawCorrection = (void(*)())GetProcAddress(OculusPlugin, "OVR_EnableMagYawCorrection");
        OVR_BeginMagAutoCalibraton = (void(*)())GetProcAddress(OculusPlugin, "OVR_EnableMagYawCorrection");
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
    //OVR_BeginMagAutoCalibraton();
    //OVR_SetSensorPredictionTime(0, 0.1);
    //OVR_EnableMagYawCorrection(0, true);
    
    MessageList messageList = {0};
    while (true) {
        uint8_t action = 1;
#ifndef DEBUG
        ReadFile(PIPE, &action, 1, NULL, NULL);
#endif

        bool exit = false;
        switch (action) {
            case 1: {
                printf("REQUEST: data\n");
                OVR_Update(&messageList);
                TUNNEL_DATA tunnel_data;
                if (!OVR_GetSensorOrientationQ(0, &tunnel_data)) {
                    printf("ERROR: failed to read sensor\n");
                }
                OVR_ProcessLatencyInputs();
                printf("RESPONSE: %f %f %f %f\n", tunnel_data.qw, tunnel_data.qx, tunnel_data.qy, tunnel_data.qz);

#ifndef DEBUG
                WriteFile(
                    PIPE,
                    &tunnel_data,
                    sizeof(TUNNEL_DATA),
                    NULL,
                    NULL
                );
#endif

                break;
            }

            case 2: {
                printf("REQUEST: reset\n");
                OVR_ResetSensorOrientation(0);
                break;
            }

            case 3: {
                printf("REQUEST: exit\n");
                exit = true;
                break;
            }
        }

        if (exit) break;
    }

    OVR_Destroy();
	return 0;
}