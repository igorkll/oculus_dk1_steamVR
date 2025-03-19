// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <openvr_driver.h>
#include <thread>
#include <atomic>
#include <cwchar>
#include <cmath>
#include <hidsdi.h>
#include <setupapi.h>
#include <iostream>
#include <vector>

using namespace vr;
using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define VR_WIDTH 1280
#define VR_HEIGHT 800
#define horizontalFOV 90

#define EYE_WIDTH VR_WIDTH / 2
#define EYE_HEIGHT VR_HEIGHT

typedef struct {
    double qw;
    double qx;
    double qy;
    double qz;
} TUNNEL_DATA;

char* tunnel_exe_path;

bool IsRiftDKMonitor(const MONITORINFOEX& monitorInfo) {
    return (wcscmp(monitorInfo.szDevice, L"Rift DK") == 0);
}

bool IsCorrentResolution(const MONITORINFOEX& monitorInfo) {
    return (monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left == VR_WIDTH &&
        monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top == VR_HEIGHT);
}

void ShowMessageBox(const wchar_t* format, ...) {
    wchar_t buffer[1024];
    va_list args;
    va_start(args, format);
    vswprintf(buffer, sizeof(buffer) / sizeof(wchar_t), format, args);
    va_end(args);
    MessageBoxW(NULL, buffer, L"Сообщение", MB_OK | MB_ICONINFORMATION);
}

RECT FindMonitor() {
    RECT foundRect = { 0 };

    EnumDisplayMonitors(NULL, NULL, [](HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) -> BOOL {
        MONITORINFOEX monitorInfo;
        monitorInfo.cbSize = sizeof(MONITORINFOEX);
        if (GetMonitorInfo(hMonitor, &monitorInfo)) {
            if (!(monitorInfo.dwFlags & MONITORINFOF_PRIMARY)) {
                if (IsRiftDKMonitor(monitorInfo) && IsCorrentResolution(monitorInfo)) {
                    RECT* rect = reinterpret_cast<RECT*>(dwData);
                    *rect = monitorInfo.rcMonitor;
                    return FALSE;
                }
            }
        }
        return TRUE;
        }, reinterpret_cast<LPARAM>(&foundRect));

    if (foundRect.left == 0 && foundRect.top == 0 && foundRect.right == 0 && foundRect.bottom == 0) {
        EnumDisplayMonitors(NULL, NULL, [](HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) -> BOOL {
            MONITORINFOEX monitorInfo;
            monitorInfo.cbSize = sizeof(MONITORINFOEX);
            if (GetMonitorInfo(hMonitor, &monitorInfo)) {
                if (!(monitorInfo.dwFlags & MONITORINFOF_PRIMARY)) {
                    if (IsCorrentResolution(monitorInfo)) {
                        RECT* rect = reinterpret_cast<RECT*>(dwData);
                        *rect = monitorInfo.rcMonitor;
                        return FALSE;
                    }
                }
            }
            return TRUE;
            }, reinterpret_cast<LPARAM>(&foundRect));
    }

    return foundRect;
}

class VRDisplay : public IVRDisplayComponent {
    void GetWindowBounds(int32_t* pnX, int32_t* pnY, uint32_t* pnWidth, uint32_t* pnHeight) {
        RECT rect = FindMonitor();
        *pnX = rect.left;
        *pnY = rect.top;
        *pnWidth = rect.right - rect.left;
        *pnHeight = rect.bottom - rect.top;
    }

    bool IsDisplayOnDesktop() {
        return true;
    }

    bool IsDisplayRealDisplay() {
        return true;
    }

    void GetRecommendedRenderTargetSize(uint32_t* pnWidth, uint32_t* pnHeight) {
        *pnWidth = EYE_WIDTH;
        *pnHeight = EYE_HEIGHT;
    }

    void GetEyeOutputViewport(EVREye eEye, uint32_t* pnX, uint32_t* pnY, uint32_t* pnWidth, uint32_t* pnHeight) {
        *pnY = 0;
        *pnWidth = EYE_WIDTH;
        *pnHeight = EYE_HEIGHT;

        if (eEye == Eye_Left)
        {
            *pnX = 0;
        }
        else
        {
            *pnX = EYE_WIDTH;
        }
    }
    
    void GetProjectionRaw(EVREye eEye, float* pfLeft, float* pfRight, float* pfTop, float* pfBottom) {
        double horfov = tan((horizontalFOV / 2) * (M_PI / 180.0));
        double verfov = horfov * (((double)(EYE_HEIGHT)) / ((double)(EYE_WIDTH)));

        if (eEye == Eye_Left)
        {
            *pfLeft = -horfov;
            *pfRight = horfov;
            *pfTop = -verfov;
            *pfBottom = verfov;
        }
        else
        {
            *pfLeft = -horfov;
            *pfRight = horfov;
            *pfTop = -verfov;
            *pfBottom = verfov;
        }
    }

    DistortionCoordinates_t ComputeDistortion(EVREye eEye, float fU, float fV) {
        DistortionCoordinates_t distortionCoordinates_t;

        double destortionFactor = 1.1;

        float rU;
        float rV = (fV * destortionFactor) - ((destortionFactor - 1.0) / 2);
        if (eEye == Eye_Left)
        {
            rU = (fU * destortionFactor) - (destortionFactor - 1.0);
        }
        else
        {
            rU = fU * destortionFactor;
        }

        distortionCoordinates_t.rfBlue[0] = rU;
        distortionCoordinates_t.rfBlue[1] = rV;
        distortionCoordinates_t.rfGreen[0] = rU;
        distortionCoordinates_t.rfGreen[1] = rV;
        distortionCoordinates_t.rfRed[0] = rU;
        distortionCoordinates_t.rfRed[1] = rV;

        return distortionCoordinates_t;
    }

    bool ComputeInverseDistortion(HmdVector2_t* pResult, EVREye eEye, uint32_t unChannel, float fU, float fV) {
        return false;
    }
};

class HeadDisplay : public ITrackedDeviceServerDriver {
private:
    VRDisplay vrDisplay;
    thread th;
    atomic<bool> isActive;
    atomic<uint32_t> deviceIndex;
    HANDLE HID;

    void threadFunc() {
        HANDLE pipe = CreateNamedPipeA(
            "\\\\.\\pipe\\OculusPluginBinding",
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE,
            1,
            0,
            0,
            0,
            NULL
        );

        if (!pipe || pipe == INVALID_HANDLE_VALUE) {
            MessageBoxA(NULL, "", "Unable to create pipe", MB_OK | MB_ICONERROR);
            return;
        }

        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
        CreateProcessA(tunnel_exe_path, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

        if (!ConnectNamedPipe(pipe, NULL)) {
            char* buffer = nullptr;
            FormatMessageA(
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                GetLastError(),
                NULL,
                reinterpret_cast<char*>(&buffer),
                0,
                nullptr
            );

            MessageBoxA(NULL, buffer, "Unable to connect named pipe", MB_OK | MB_ICONERROR);
            LocalFree(buffer);
            return;
        }

        while (isActive) {
            uint8_t sendByte = 1;
            WriteFile(pipe, &sendByte, 1, NULL, NULL);

            TUNNEL_DATA tunnel_data;
            ReadFile(pipe, &tunnel_data, sizeof(tunnel_data), NULL, NULL);

            DriverPose_t pose = { 0 };
            pose.qWorldFromDriverRotation.w = 1.f;
            pose.qDriverFromHeadRotation.w = 1.f;
            pose.qRotation.x = tunnel_data.qx;
            pose.qRotation.y = tunnel_data.qy;
            pose.qRotation.z = tunnel_data.qz;
            pose.qRotation.w = tunnel_data.qw;
            pose.vecPosition[0] = 0.0f;
            pose.vecPosition[1] = 1.77f + (tunnel_data.qy * 3);
            pose.vecPosition[2] = 0.0f;
            pose.poseIsValid = true;
            pose.deviceIsConnected = true;
            pose.result = TrackingResult_Running_OK;
            pose.shouldApplyHeadModel = true;
            VRServerDriverHost()->TrackedDevicePoseUpdated(deviceIndex, pose, sizeof(DriverPose_t));
            this_thread::sleep_for(chrono::milliseconds(5));
        }

        uint8_t sendByte = 0;
        WriteFile(pipe, &sendByte, 1, NULL, NULL);
    }

public:
    HeadDisplay() {
        isActive = false;
        deviceIndex = k_unTrackedDeviceIndexInvalid;
    }

    EVRInitError Activate(uint32_t unObjectId) {
        isActive = true;
        deviceIndex = unObjectId;

        vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(unObjectId);
        vr::VRProperties()->SetBoolProperty(container, vr::Prop_IsOnDesktop_Bool, true);
        vr::VRProperties()->SetBoolProperty(container, vr::Prop_DisplayDebugMode_Bool, false);

        th = thread(&HeadDisplay::threadFunc, this);
        return VRInitError_None;
    }

    void Deactivate() {
        if (isActive.exchange(false)) {
            th.join();
        }

        deviceIndex = k_unTrackedDeviceIndexInvalid;
    }

    void EnterStandby() {
        
    }

    void* GetComponent(const char* pchComponentNameAndVersion) {
        if (strcmp(pchComponentNameAndVersion, IVRDisplayComponent_Version) == 0) {
            return &vrDisplay;
        }

        return nullptr;
    }

    void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) {
        if (unResponseBufferSize >= 1)
            pchResponseBuffer[0] = 0;
    }

    DriverPose_t GetPose() {
        DriverPose_t pose = { 0 };

        pose.qWorldFromDriverRotation.w = 1.f;
        pose.qDriverFromHeadRotation.w = 1.f;

        pose.qRotation.w = 1.f;

        pose.vecPosition[0] = 0.0f;
        pose.vecPosition[1] = 0.0f;
        pose.vecPosition[2] = 0.0f;

        pose.poseIsValid = true;
        pose.deviceIsConnected = true;
        pose.result = TrackingResult_Running_OK;
        pose.shouldApplyHeadModel = true;
        return pose;
    }
};

class MyServerTrackedDeviceProvider : public IServerTrackedDeviceProvider {
    HeadDisplay* headDisplay;

    EVRInitError Init(IVRDriverContext* pDriverContext) {
        VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
        headDisplay = new HeadDisplay();
        if (!VRServerDriverHost()->TrackedDeviceAdded("HEAD_DISPLAY", TrackedDeviceClass_HMD, headDisplay)) {
            return VRInitError_Driver_Unknown;
        }
        return VRInitError_None;
    }

    void Cleanup() {
        delete headDisplay;
    }

    const char* const* GetInterfaceVersions() {
        return k_InterfaceVersions;
    }

    void RunFrame() {
        VREvent_t vrevent{};
        while (VRServerDriverHost()->PollNextEvent(&vrevent, sizeof(VREvent_t)))
        {

        }
    }

    bool ShouldBlockStandbyMode() { //legacy
        return false;
    }

    void EnterStandby() {
        
    }

    void LeaveStandby() {

    }
};

MyServerTrackedDeviceProvider device_provider;

extern "C" __declspec(dllexport)
void* HmdDriverFactory(const char* pInterfaceName, int* pReturnCode)
{
    if (0 == strcmp(IServerTrackedDeviceProvider_Version, pInterfaceName))
    {
        return &device_provider;
    }

    if (pReturnCode)
        *pReturnCode = VRInitError_Init_InterfaceNotFound;

    return NULL;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: {
        char dllPath[MAX_PATH];
        GetModuleFileNameA(hModule, dllPath, sizeof(dllPath));

        std::string directory(dllPath);
        size_t pos = directory.find_last_of("\\/");
        if (pos != std::string::npos) {
            directory = directory.substr(0, pos + 1);
        }

        std::string exePath = directory + "oculus_tunnel_32.exe";

        const char* cstr = exePath.c_str();
        size_t cstrlen = strlen(cstr) + 1;
        tunnel_exe_path = (char*)malloc(cstrlen);
        memcpy(tunnel_exe_path, cstr, cstrlen);
    }

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

