// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <openvr_driver.h>

using namespace vr;

class MyServerTrackedDeviceProvider : public IServerTrackedDeviceProvider {
    EVRInitError Init(IVRDriverContext* pDriverContext) {
        VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
        vr::VRServerDriverHost()->TrackedDeviceAdded("HEAD_DISPLAY", vr::TrackedDeviceClass_HMD, NULL);
        return VRInitError_None;
    }

    void Cleanup() {

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

