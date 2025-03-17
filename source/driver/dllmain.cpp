// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <openvr_driver.h>

using namespace vr;

class HeadDisplay : public ITrackedDeviceServerDriver {
    EVRInitError Activate(uint32_t unObjectId) {
        return VRInitError_None;
    }

    void Deactivate() {

    }

    void EnterStandby() {

    }

    void* GetComponent(const char* pchComponentNameAndVersion) {
        if (!_stricmp(pchComponentNameAndVersion, vr::IVRVirtualDisplay_Version)) {
            return NULL;
        }

        if (!_stricmp(pchComponentNameAndVersion, vr::IVRCameraComponent_Version)) {
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
        vr::VRServerDriverHost()->TrackedDeviceAdded("HEAD_DISPLAY", vr::TrackedDeviceClass_HMD, headDisplay);
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

