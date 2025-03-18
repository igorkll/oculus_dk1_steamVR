// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <openvr_driver.h>
#include <thread>
#include <atomic>
#include <cwchar>

using namespace vr;
using namespace std;

#define VR_WIDTH 1280
#define VR_HEIGHT 800

bool IsRiftDKMonitor(const MONITORINFOEX& monitorInfo) {
    return (wcscmp(monitorInfo.szDevice, L"Rift DK") == 0);
}

bool IsCorrentResolution(const MONITORINFOEX& monitorInfo) {
    return (monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left == VR_WIDTH &&
        monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top == VR_HEIGHT);
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
        *pnWidth = VR_WIDTH / 2;
        *pnHeight = VR_HEIGHT;
    }

    void GetEyeOutputViewport(EVREye eEye, uint32_t* pnX, uint32_t* pnY, uint32_t* pnWidth, uint32_t* pnHeight) {
        *pnY = 0;
        *pnWidth = VR_WIDTH / 2;
        *pnHeight = VR_HEIGHT;

        if (eEye == Eye_Left)
        {
            *pnX = 0;
        }
        else
        {
            *pnX = VR_WIDTH / 2;
        }
    }
    
    void GetProjectionRaw(EVREye eEye, float* pfLeft, float* pfRight, float* pfTop, float* pfBottom) {
        *pfLeft = -1.0f;
        *pfRight = 1.0f;
        *pfTop = -1.0f;
        *pfBottom = 1.0f;
    }

    DistortionCoordinates_t ComputeDistortion(EVREye eEye, float fU, float fV) {
        DistortionCoordinates_t distortionCoordinates_t;
        distortionCoordinates_t.rfBlue[0] = fU;
        distortionCoordinates_t.rfBlue[1] = fV;
        distortionCoordinates_t.rfGreen[0] = fU;
        distortionCoordinates_t.rfGreen[1] = fV;
        distortionCoordinates_t.rfRed[0] = fU;
        distortionCoordinates_t.rfRed[1] = fV;
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

    void threadFunc() {
        while (isActive)
        {
            VRServerDriverHost()->TrackedDevicePoseUpdated(deviceIndex, GetPose(), sizeof(DriverPose_t));
            this_thread::sleep_for(chrono::milliseconds(5));
        }
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
        pose.vecPosition[1] = sin(0 * 0.01) * 0.1f + 1.0f;
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
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

