#include "WgpContext.h"

WgpContext wgpContext = {};

void wgpInit(void* window) {
	wgpCreateDevice(window);
}

void wgpCreateDevice(void* window) {
	WGPUInstanceDescriptor instanceDescriptor = {};
    wgpContext.instance = wgpuCreateInstance(&instanceDescriptor);

    /*WGPURequestAdapterOptions requestAdapterOptions = {};
    requestAdapterOptions.compatibleSurface = wgpContext.surface;
    requestAdapterOptions.forceFallbackAdapter = false;
    requestAdapterOptions.powerPreference = WGPUPowerPreference_HighPerformance;
    requestAdapterOptions.backendType = WGPUBackendType_Vulkan;

    WGPURequestAdapterCallbackInfo requestAdapterCallbackInfo = {};
    requestAdapterCallbackInfo.callback = OnRequestAdapter;
    requestAdapterCallbackInfo.userdata1 = &wgpContext;
    requestAdapterCallbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    WGPUFuture futureAdapter = wgpuInstanceRequestAdapter(wgpContext.instance, &requestAdapterOptions, requestAdapterCallbackInfo);

    WGPURequestDeviceCallbackInfo  deviceCallbackInfo = {};
    deviceCallbackInfo.callback = OnRequestDevice;
    deviceCallbackInfo.userdata1 = &wgpContext;
    deviceCallbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;

    WGPULimits requiredLimits = {};
    setDefault(requiredLimits);
    requiredLimits.maxTextureDimension1D = 2048;
    requiredLimits.maxTextureDimension2D = 2048;
    requiredLimits.maxTextureDimension3D = 2048;
    requiredLimits.maxSamplersPerShaderStage = 1;

    WGPUUncapturedErrorCallbackInfo errorCallbackInfo = {};
    errorCallbackInfo.callback = OnErrorDevice;

    std::vector<WGPUFeatureName> deviceFeatures;
    deviceFeatures.push_back(WGPUFeatureName::WGPUFeatureName_PrimitiveIndex);

    WGPUDeviceDescriptor deviceDescriptor = {};
    deviceDescriptor.requiredLimits = &requiredLimits;
    deviceDescriptor.uncapturedErrorCallbackInfo = errorCallbackInfo;
    deviceDescriptor.requiredFeatures = deviceFeatures.data();
    deviceDescriptor.requiredFeatureCount = deviceFeatures.size();

    WGPUFuture futureDevice = wgpuAdapterRequestDevice(wgpContext.adapter, &deviceDescriptor, deviceCallbackInfo);

    WGPUSurfaceSourceAndroidNativeWindow androidSource = {};
    androidSource.chain.sType = WGPUSType_SurfaceSourceAndroidNativeWindow;
    androidSource.chain.next = NULL;
    androidSource.window = window;

    WGPUSurfaceDescriptor surfaceDesc = {};
    surfaceDesc.label = "Android Surface";
    surfaceDesc.nextInChain = (const WGPUChainedStruct*)&androidSource;

    WGPUSurface surface = wgpuInstanceCreateSurface(instance, &surfaceDesc);*/

}