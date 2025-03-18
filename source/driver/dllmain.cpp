// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <openvr_driver.h>
#include <thread>
#include <atomic>

using namespace vr;
using namespace std;

#define VR_WIDTH 1280
#define VR_HEIGHT 720

class VRDisplay : public IVRDisplayComponent {
    void GetWindowBounds(int32_t* pnX, int32_t* pnY, uint32_t* pnWidth, uint32_t* pnHeight) {
        *pnX = 0;
        *pnY = 0;
        *pnWidth = VR_WIDTH;
        *pnHeight = VR_HEIGHT;
    }

    bool IsDisplayOnDesktop() {
        return false;
    }

    bool IsDisplayRealDisplay() {
        return true;
    }

    void GetRecommendedRenderTargetSize(uint32_t* pnWidth, uint32_t* pnHeight) {
        *pnWidth = VR_WIDTH;
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
    VRDisplay* vrDisplay;
    thread th;
    atomic<bool> isActive = false;
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
        vrDisplay = new VRDisplay();
    }

    EVRInitError Activate(uint32_t unObjectId) {
        isActive = true;
        deviceIndex = unObjectId;

        vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(deviceIndex);

        // Let's begin setting up the properties now we've got our container.
        // A list of properties available is contained in vr::ETrackedDeviceProperty.

        // Next, display settings

        // Get the ipd of the user from SteamVR settings
        const float ipd = vr::VRSettings()->GetFloat(vr::k_pch_SteamVR_Section, vr::k_pch_SteamVR_IPD_Float);
        vr::VRProperties()->SetFloatProperty(container, vr::Prop_UserIpdMeters_Float, ipd);

        // For HMDs, it's required that a refresh rate is set otherwise VRCompositor will fail to start.
        vr::VRProperties()->SetFloatProperty(container, vr::Prop_DisplayFrequency_Float, 0.f);

        // The distance from the user's eyes to the display in meters. This is used for reprojection.
        vr::VRProperties()->SetFloatProperty(container, vr::Prop_UserHeadToEyeDepthMeters_Float, 0.f);

        // How long from the compositor to submit a frame to the time it takes to display it on the screen.
        vr::VRProperties()->SetFloatProperty(container, vr::Prop_SecondsFromVsyncToPhotons_Float, 0.11f);

        // avoid "not fullscreen" warnings from vrmonitor
        vr::VRProperties()->SetBoolProperty(container, vr::Prop_IsOnDesktop_Bool, false);

        vr::VRProperties()->SetBoolProperty(container, vr::Prop_DisplayDebugMode_Bool, true);

        th = thread(&HeadDisplay::threadFunc, this);
        return VRInitError_None;
    }

    void Deactivate() {
        if (isActive.exchange(false)) {
            th.join();
        }
    }

    void EnterStandby() {
        
    }

    void* GetComponent(const char* pchComponentNameAndVersion) {
        if (strcmp(pchComponentNameAndVersion, IVRDisplayComponent_Version) == 0) {
            return vrDisplay;
        }

        return nullptr;
    }

    void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) {
        if (unResponseBufferSize >= 1)
            pchResponseBuffer[0] = 0;
    }

    DriverPose_t GetPose() { //legacy
        DriverPose_t t;
        return t;
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
        vr::VREvent_t vrevent{};
        while (vr::VRServerDriverHost()->PollNextEvent(&vrevent, sizeof(vr::VREvent_t)))
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

class MyWatchdogProvider : public IVRWatchdogProvider {
    EVRInitError Init(IVRDriverContext* pDriverContext) {
        return VRInitError_None;
    }

    void Cleanup() {

    }
};

MyServerTrackedDeviceProvider device_provider;
MyWatchdogProvider watchdog_provider;

extern "C" __declspec(dllexport)
void* HmdDriverFactory(const char* pInterfaceName, int* pReturnCode)
{
    if (0 == strcmp(IServerTrackedDeviceProvider_Version, pInterfaceName))
    {
        return &device_provider;
    }

    if (0 == strcmp(IVRWatchdogProvider_Version, pInterfaceName))
    {
        return &watchdog_provider;
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

