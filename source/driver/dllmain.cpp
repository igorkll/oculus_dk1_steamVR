// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <openvr_driver.h>

using namespace vr;

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
        return true;
    }

    bool IsDisplayRealDisplay() {
        return false;
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
public:
    VRDisplay* vrDisplay;

    HeadDisplay() {
        vrDisplay = new VRDisplay();
    }

    EVRInitError Activate(uint32_t unObjectId) {
        PropertyContainerHandle_t container = VRProperties()->TrackedDeviceToPropertyContainer(unObjectId);
        VRProperties()->SetStringProperty(container, Prop_ModelNumber_String, "oculus devkit 1");

        return VRInitError_None;
    }

    void Deactivate() {
    }

    void EnterStandby() {
        
    }

    void* GetComponent(const char* pchComponentNameAndVersion) {
        if (!_stricmp(pchComponentNameAndVersion, IVRVirtualDisplay_Version)) {
            return vrDisplay;
        }

        if (!_stricmp(pchComponentNameAndVersion, IVRCameraComponent_Version)) {
            return NULL;
        }

        return nullptr;
    }

    void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) {

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
        VRServerDriverHost()->TrackedDeviceAdded("HEAD_DISPLAY", TrackedDeviceClass_HMD, headDisplay);
        return VRInitError_None;
    }

    void Cleanup() {
        delete headDisplay;
    }

    const char* const* GetInterfaceVersions() {
        return k_InterfaceVersions;
    }

    void RunFrame() {

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

