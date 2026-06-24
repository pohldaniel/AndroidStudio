#include <android/native_window.h>
#include "Logging.h"
#include "WgpContext.h"

WgpContext wgpContext = {};
std::unordered_map<VertexLayoutSlot, std::vector<WGPUVertexAttribute>> wgpVertexAttributes;
std::unordered_map<VertexLayoutSlot, std::vector<WGPUVertexBufferLayout>> wgpVertexBufferLayouts;

void OnRequestAdapter(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void* userdata1, void* userdata2) {
    struct std::pair<WgpContext&, bool>* userdata = (std::pair<WgpContext&, bool>*)userdata1;
    if (status == WGPURequestAdapterStatus_Success) {
        userdata->first.adapter = adapter;
	}else {
		LOGE("Adapter message: %s", message.data);
	}
    userdata->second = true;
}

void OnRequestDevice(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void* userdata1, void* userdata2) {
    std::pair<WgpContext&, bool>* userdata = reinterpret_cast<std::pair<WgpContext&, bool>*>(userdata1);
    if (status == WGPURequestDeviceStatus_Success) {
        userdata->first.device = device;
	}else {
		LOGE("Device message: %s", message.data);
	}
    userdata->second = true;
}

void OnErrorDevice(const WGPUDevice* device, WGPUErrorType type, WGPUStringView message, void* userdata1, void* userdata2) {
	LOGE("Error message: %s", message.data);
}

void wgpRequestAdapterSync(WGPUInstance instance, const WGPURequestAdapterOptions* requestAdapterOptions) {
	std::pair<WgpContext&, bool> userdata = {wgpContext, false};
	WGPURequestAdapterCallbackInfo requestAdapterCallbackInfo = {};
	requestAdapterCallbackInfo.callback = OnRequestAdapter;
	requestAdapterCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
	requestAdapterCallbackInfo.userdata1 = &userdata;
	wgpuInstanceRequestAdapter(instance, requestAdapterOptions, requestAdapterCallbackInfo);
}

void wgpRequestDeviceSync(WGPUAdapter adapter, const WGPUDeviceDescriptor* deviceDescriptor) {
    std::pair<WgpContext&, bool> userdata = {wgpContext, false};
    WGPURequestDeviceCallbackInfo requestDeviceCallbackInfo = {};
    requestDeviceCallbackInfo.callback = OnRequestDevice;
    requestDeviceCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
    requestDeviceCallbackInfo.userdata1 = &userdata;
    wgpuAdapterRequestDevice(adapter, deviceDescriptor, requestDeviceCallbackInfo);
}

void setDefault(WGPULimits& limits) {
	limits.maxTextureDimension1D = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxTextureDimension2D = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxTextureDimension3D = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxTextureArrayLayers = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxBindGroups = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxBindGroupsPlusVertexBuffers = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxBindingsPerBindGroup = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxDynamicUniformBuffersPerPipelineLayout = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxDynamicStorageBuffersPerPipelineLayout = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxSampledTexturesPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxSamplersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxStorageBuffersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxStorageTexturesPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxUniformBuffersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxUniformBufferBindingSize = WGPU_LIMIT_U64_UNDEFINED;
	limits.maxStorageBufferBindingSize = WGPU_LIMIT_U64_UNDEFINED;
	limits.minUniformBufferOffsetAlignment = WGPU_LIMIT_U32_UNDEFINED;
	limits.minStorageBufferOffsetAlignment = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxVertexBuffers = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxBufferSize = WGPU_LIMIT_U64_UNDEFINED;
	limits.maxVertexAttributes = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxVertexBufferArrayStride = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxInterStageShaderVariables = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxColorAttachments = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxColorAttachmentBytesPerSample = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxComputeWorkgroupStorageSize = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxComputeInvocationsPerWorkgroup = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxComputeWorkgroupSizeX = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxComputeWorkgroupSizeY = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxComputeWorkgroupSizeZ = WGPU_LIMIT_U32_UNDEFINED;
	limits.maxComputeWorkgroupsPerDimension = WGPU_LIMIT_U32_UNDEFINED;
}

void setDefault(WGPUStencilFaceState& stencilFaceState) {
	stencilFaceState.compare = WGPUCompareFunction::WGPUCompareFunction_Always;
	stencilFaceState.failOp = WGPUStencilOperation::WGPUStencilOperation_Keep;
	stencilFaceState.depthFailOp = WGPUStencilOperation::WGPUStencilOperation_Keep;
	stencilFaceState.passOp = WGPUStencilOperation::WGPUStencilOperation_Keep;
}

void setDefault(WGPUDepthStencilState& depthStencilState) {
	depthStencilState.nextInChain = nullptr;
	depthStencilState.format = WGPUTextureFormat::WGPUTextureFormat_Undefined;
	depthStencilState.depthWriteEnabled = WGPUOptionalBool::WGPUOptionalBool_True;
	depthStencilState.depthCompare = WGPUCompareFunction::WGPUCompareFunction_Less;
	setDefault(depthStencilState.stencilFront);
	setDefault(depthStencilState.stencilBack);
	depthStencilState.stencilReadMask = 0;
	depthStencilState.stencilWriteMask = 0;
	depthStencilState.depthBias = 0;
	depthStencilState.depthBiasSlopeScale = 0.0f;
	depthStencilState.depthBiasClamp = 0.0f;
}

void setDefault(WGPUBindGroupLayoutEntry& bindingLayout) {
	bindingLayout.buffer.nextInChain = nullptr;
	bindingLayout.buffer.type = WGPUBufferBindingType_Undefined;
	bindingLayout.buffer.hasDynamicOffset = false;

	bindingLayout.sampler.nextInChain = nullptr;
	bindingLayout.sampler.type = WGPUSamplerBindingType_Undefined;

	bindingLayout.storageTexture.nextInChain = nullptr;
	bindingLayout.storageTexture.access = WGPUStorageTextureAccess_Undefined;
	bindingLayout.storageTexture.format = WGPUTextureFormat_Undefined;
	bindingLayout.storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;

	bindingLayout.texture.nextInChain = nullptr;
	bindingLayout.texture.multisampled = false;
	bindingLayout.texture.sampleType = WGPUTextureSampleType_Undefined;
	bindingLayout.texture.viewDimension = WGPUTextureViewDimension_Undefined;
}

void wgpInit() {
	wgpCreateDevice();
}

void wgpCreateDevice() {

	WGPUInstanceExtras instanceExtras = {};
	instanceExtras.chain.sType = (WGPUSType)WGPUSType_InstanceExtras;
	instanceExtras.chain.next = nullptr;
	instanceExtras.backends = WGPUInstanceBackend_Primary;

	std::vector<WGPUInstanceFeatureName> instanceFeatures;
    instanceFeatures.push_back(WGPUInstanceFeatureName::WGPUInstanceFeatureName_TimedWaitAny);
	WGPUInstanceDescriptor instanceDescriptor = {};
	instanceDescriptor.nextInChain = &instanceExtras.chain;
	instanceDescriptor.requiredFeatureCount = instanceFeatures.size();
	instanceDescriptor.requiredFeatures = instanceFeatures.data();
	wgpContext.instance = wgpuCreateInstance(nullptr);

	WGPURequestAdapterOptions requestAdapterOptions = {};
	requestAdapterOptions.nextInChain = nullptr;
    requestAdapterOptions.compatibleSurface = nullptr;
	requestAdapterOptions.powerPreference = WGPUPowerPreference_HighPerformance;
	requestAdapterOptions.backendType = WGPUBackendType_Vulkan;
    requestAdapterOptions.featureLevel = WGPUFeatureLevel_Undefined;

    wgpRequestAdapterSync(wgpContext.instance, &requestAdapterOptions);

    WGPULimits requiredLimits = {};
    setDefault(requiredLimits);
    requiredLimits.nextInChain = nullptr;
    requiredLimits.maxTextureDimension1D = 4096;
    requiredLimits.maxTextureDimension2D = 4096;
    requiredLimits.maxTextureDimension3D = 2048u;
    requiredLimits.maxSamplersPerShaderStage = 1u;

    WGPUUncapturedErrorCallbackInfo uncapturedErrorCallbackInfo = {};
    uncapturedErrorCallbackInfo.callback = OnErrorDevice;

    WGPUDeviceDescriptor deviceDescriptor = {};
    deviceDescriptor.requiredLimits = &requiredLimits;
    deviceDescriptor.uncapturedErrorCallbackInfo = uncapturedErrorCallbackInfo;
    deviceDescriptor.nextInChain = nullptr;
    deviceDescriptor.label = WGPU_STR("device");
    deviceDescriptor.requiredFeatureCount = 0u;
    deviceDescriptor.requiredFeatures = nullptr;
    deviceDescriptor.defaultQueue.nextInChain = nullptr;
    deviceDescriptor.defaultQueue.label = WGPU_STR("queue");
    wgpRequestDeviceSync(wgpContext.adapter, &deviceDescriptor);

    wgpContext.queue = wgpuDeviceGetQueue(wgpContext.device);
    //wgpContext.depthTexture = wgpCreateTexture(ANativeWindow_getWidth(static_cast<ANativeWindow*>(window)), ANativeWindow_getHeight(static_cast<ANativeWindow*>(window)), 1u, WGPUTextureUsage_RenderAttachment, wgpContext.depthFormat, 1u, wgpContext.msaaSampleCount, wgpContext.depthFormat);
    //wgpContext.depthTextureView = wgpCreateTextureView(wgpContext.depthTexture, WGPUTextureAspect::WGPUTextureAspect_All);

    wgpCreateVertexBufferLayout(VL_P);
    wgpCreateVertexBufferLayout(VL_PT);
    wgpCreateVertexBufferLayout(VL_PN);
    wgpCreateVertexBufferLayout(VL_PNC);
    wgpCreateVertexBufferLayout(VL_PTN);
    wgpCreateVertexBufferLayout(VL_PTNC);
    wgpCreateVertexBufferLayout(VL_PTNTB);
    wgpCreateVertexBufferLayout(VL_PTNWJ);
    wgpCreateVertexBufferLayout(VL_BATCH);

    wgpContext.addSampler(wgpCreateSampler(WGPUFilterMode_Linear, WGPUAddressMode_ClampToEdge), SS_LINEAR_CLAMP);
    wgpContext.addSampler(wgpCreateSampler(WGPUFilterMode_Linear, WGPUAddressMode_Repeat), SS_LINEAR_REPEAT);
    wgpContext.addSampler(wgpCreateSampler(WGPUFilterMode_Nearest, WGPUAddressMode_ClampToEdge), SS_NEAREST_CLAMP);
    wgpContext.addSampler(wgpCreateSampler(WGPUFilterMode_Nearest, WGPUAddressMode_Repeat), SS_NEAREST_REPEAT);
}

void wgpConfigureSurface(void* window) {
    if(window) {
        int32_t width = ANativeWindow_getWidth(static_cast<ANativeWindow*>(window));
        int32_t height = ANativeWindow_getHeight(static_cast<ANativeWindow*>(window));

        wgpCreateSurface(window);

        wgpContext.config = {};
        wgpContext.config.nextInChain = nullptr;
        wgpContext.config.format = wgpContext.surfaceCapabilities.formats[0];
        wgpContext.config.width = width;
        wgpContext.config.height = height;
        wgpContext.config.usage = WGPUTextureUsage_RenderAttachment;
        wgpContext.config.viewFormatCount = 0;
        wgpContext.config.viewFormats = nullptr;
        wgpContext.config.device = wgpContext.device;
        wgpContext.config.presentMode = wgpContext.surfaceCapabilities.presentModes[0];
        wgpContext.config.alphaMode = wgpContext.surfaceCapabilities.alphaModes[0];
        wgpuSurfaceConfigure(wgpContext.surface, &wgpContext.config);
    }
}

void wgpResize(void* window, uint32_t width, uint32_t height) {
    if (window) {
        wgpCreateSurface(window);

        if(wgpContext.depthTexture) {
            wgpuTextureViewRelease(wgpContext.depthTextureView);
            wgpuTextureDestroy(wgpContext.depthTexture);
            wgpuTextureRelease(wgpContext.depthTexture);
        }

        wgpContext.depthTexture = wgpCreateTexture(width, height, 1u, WGPUTextureUsage_RenderAttachment, wgpContext.depthFormat, 1u, wgpContext.msaaSampleCount, wgpContext.depthFormat);
        wgpContext.depthTextureView = wgpCreateTextureView(wgpContext.depthTexture, WGPUTextureAspect::WGPUTextureAspect_All);

        if (wgpContext.msaaSampleCount > 1u) {
            wgpuTextureViewRelease(wgpContext.msaaTextureView);
            wgpuTextureDestroy(wgpContext.msaaTexture);
            wgpuTextureRelease(wgpContext.msaaTexture);

            wgpContext.msaaTexture = wgpCreateTexture(width, height, 1u, WGPUTextureUsage_RenderAttachment, wgpContext.colorFormat, 1u, wgpContext.msaaSampleCount, wgpContext.colorFormat);
            wgpContext.msaaTextureView = wgpCreateTextureView(wgpContext.msaaTexture, WGPUTextureAspect::WGPUTextureAspect_All);
        }

        wgpContext.config.width = width;
        wgpContext.config.height = height;
        wgpuSurfaceConfigure(wgpContext.surface, &wgpContext.config);
    }
}

WGPUTexture wgpCreateTexture(uint32_t width, uint32_t height, uint32_t depth, WGPUTextureUsage textureUsage, WGPUTextureFormat textureFormat, uint32_t mipLevelCount, uint32_t sampleCount, WGPUTextureFormat viewFormat) {
    const WGPUDevice& device = wgpContext.device;
    WGPUTextureDescriptor textureDescriptor = {};
    textureDescriptor.label = WGPU_STR("texture");
    textureDescriptor.dimension = WGPUTextureDimension::WGPUTextureDimension_2D;
    textureDescriptor.size = { width, height, depth };
    textureDescriptor.format = textureFormat;
    textureDescriptor.usage = textureUsage;
    textureDescriptor.mipLevelCount = mipLevelCount;
    textureDescriptor.sampleCount = sampleCount;
    textureDescriptor.nextInChain = nullptr;
    if (viewFormat != WGPUTextureFormat_Undefined) {
        textureDescriptor.viewFormatCount = 1u;
        textureDescriptor.viewFormats = &viewFormat;
    }
    return wgpuDeviceCreateTexture(device, &textureDescriptor);
}

WGPUTextureView wgpCreateTextureView(const WGPUTexture& texture, WGPUTextureAspect aspect) {
    WGPUTextureViewDescriptor textureViewDescriptor = {};
    textureViewDescriptor.label = WGPU_STR("texture_view");
    textureViewDescriptor.aspect = aspect;
    textureViewDescriptor.baseArrayLayer = 0u;
    textureViewDescriptor.arrayLayerCount = wgpuTextureGetDepthOrArrayLayers(texture);
    textureViewDescriptor.baseMipLevel = 0u;
    textureViewDescriptor.mipLevelCount = wgpuTextureGetMipLevelCount(texture);
    textureViewDescriptor.dimension = textureViewDescriptor.arrayLayerCount == 6u ? WGPUTextureViewDimension::WGPUTextureViewDimension_Cube : WGPUTextureViewDimension::WGPUTextureViewDimension_2D;
    textureViewDescriptor.format = wgpuTextureGetFormat(texture);
    textureViewDescriptor.nextInChain = nullptr;
    return wgpuTextureCreateView(texture, &textureViewDescriptor);
}

WGPUBuffer wgpCreateEmptyBuffer(uint32_t size, WGPUBufferUsage bufferUsage, bool mappedAtCreation) {
    const WGPUDevice& device = wgpContext.device;
    WGPUBufferDescriptor bufferDesc = {};
    bufferDesc.label = WGPU_STR("buf");

    if (bufferUsage & WGPUBufferUsage_Uniform)
        bufferDesc.label = WGPU_STR("uniform_buf");

    if (bufferUsage & WGPUBufferUsage_Vertex)
        bufferDesc.label = WGPU_STR("vertex_buf");

    if (bufferUsage & WGPUBufferUsage_Index)
        bufferDesc.label = WGPU_STR("index_buf");

    if (bufferUsage & WGPUBufferUsage_Storage)
        bufferDesc.label = WGPU_STR("storage_buf");

    bufferDesc.size = size;
    bufferDesc.usage = bufferUsage;
    bufferDesc.mappedAtCreation = mappedAtCreation;
    return wgpuDeviceCreateBuffer(device, &bufferDesc);
}

WGPUBuffer wgpCreateBuffer(const void* data, uint32_t size, WGPUBufferUsage bufferUsage) {
    const WGPUDevice& device = wgpContext.device;
    WGPUBufferDescriptor bufferDesc = {};
    bufferDesc.label = WGPU_STR("buf");

    if (bufferUsage & WGPUBufferUsage_Uniform)
        bufferDesc.label = WGPU_STR("uniform_buf");

    if (bufferUsage & WGPUBufferUsage_Vertex)
        bufferDesc.label = WGPU_STR("vertex_buf");

    if (bufferUsage & WGPUBufferUsage_Index)
        bufferDesc.label = WGPU_STR("index_buf");

    if (bufferUsage & WGPUBufferUsage_Storage)
        bufferDesc.label = WGPU_STR("storage_buf");

    bufferDesc.size = size;
    bufferDesc.usage = bufferUsage;
    bufferDesc.mappedAtCreation = true;

    WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
    void* mapping = wgpuBufferGetMappedRange(buffer, 0, size);
    memcpy(mapping, data, size);
    wgpuBufferUnmap(buffer);
    return buffer;
}

WGPUShaderModule wgpCreateShaderFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return nullptr;
    }
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    std::string shaderSource(size, ' ');
    file.seekg(0);
    file.read(shaderSource.data(), size);
    file.close();

    WGPUShaderSourceWGSL shaderSourceWGSL = {};
    shaderSourceWGSL.chain.next = nullptr;
    shaderSourceWGSL.chain.sType = WGPUSType_ShaderSourceWGSL;
    shaderSourceWGSL.code = { shaderSource.c_str(), WGPU_STRLEN };

    WGPUShaderModuleDescriptor shaderModuleDescriptor = {};
    shaderModuleDescriptor.label = { path.c_str(), path.length() };
    shaderModuleDescriptor.nextInChain = &shaderSourceWGSL.chain;

    return wgpuDeviceCreateShaderModule(wgpContext.device, &shaderModuleDescriptor);
}

WGPUShaderModule wgpCreateShaderFromString(const std::string& string) {
    WGPUShaderSourceWGSL shaderSourceWGSL = {};
    shaderSourceWGSL.chain.next = nullptr;
    shaderSourceWGSL.chain.sType = WGPUSType_ShaderSourceWGSL;
    shaderSourceWGSL.code = { string.c_str(), WGPU_STRLEN };

    WGPUShaderModuleDescriptor shaderModuleDescriptor = {};
    shaderModuleDescriptor.label = WGPU_STR("shader");
    shaderModuleDescriptor.nextInChain = &shaderSourceWGSL.chain;

    return wgpuDeviceCreateShaderModule(wgpContext.device, &shaderModuleDescriptor);
}

std::vector<WGPUVertexAttribute>& wgpVertexAttribute(VertexLayoutSlot vertexLayoutSlot) {
    return wgpVertexAttributes[vertexLayoutSlot];
}

std::vector<WGPUVertexBufferLayout>& wgpVertexBufferLayout(VertexLayoutSlot vertexLayoutSlot) {
    return wgpVertexBufferLayouts[vertexLayoutSlot];
}

void wgpCreateVertexBufferLayout(VertexLayoutSlot slot) {
    if (wgpVertexBufferLayouts.count(VL_P) == 0 && slot == VL_P) {
        std::vector<WGPUVertexAttribute>& wgpVertexAttribute = wgpVertexAttributes[VL_P];
        wgpVertexAttribute.resize(1);

        wgpVertexAttribute[0].shaderLocation = 0;
        wgpVertexAttribute[0].format = WGPUVertexFormat::WGPUVertexFormat_Float32x3;
        wgpVertexAttribute[0].offset = 0;

        WGPUVertexBufferLayout wgpVertexBufferLayout = {};
        wgpVertexBufferLayout.attributeCount = (uint32_t)wgpVertexAttribute.size();
        wgpVertexBufferLayout.attributes = wgpVertexAttribute.data();
        wgpVertexBufferLayout.arrayStride = 3 * sizeof(float);
        wgpVertexBufferLayout.stepMode = WGPUVertexStepMode::WGPUVertexStepMode_Vertex;
        wgpVertexBufferLayouts[VL_P].push_back(wgpVertexBufferLayout);
    }else if (wgpVertexBufferLayouts.count(VL_PT) == 0 && slot == VL_PT) {
        std::vector<WGPUVertexAttribute>& wgpVertexAttribute = wgpVertexAttributes[VL_PT];
        wgpVertexAttribute.resize(2);

        wgpVertexAttribute[0].shaderLocation = 0;
        wgpVertexAttribute[0].format = WGPUVertexFormat::WGPUVertexFormat_Float32x3;
        wgpVertexAttribute[0].offset = 0;

        wgpVertexAttribute[1].shaderLocation = 1;
        wgpVertexAttribute[1].format = WGPUVertexFormat::WGPUVertexFormat_Float32x2;
        wgpVertexAttribute[1].offset = 3 * sizeof(float);

        WGPUVertexBufferLayout wgpVertexBufferLayout = {};
        wgpVertexBufferLayout.attributeCount = (uint32_t)wgpVertexAttribute.size();
        wgpVertexBufferLayout.attributes = wgpVertexAttribute.data();
        wgpVertexBufferLayout.arrayStride = 5 * sizeof(float);
        wgpVertexBufferLayout.stepMode = WGPUVertexStepMode::WGPUVertexStepMode_Vertex;
        wgpVertexBufferLayouts[VL_PT].push_back(wgpVertexBufferLayout);
    }else if (wgpVertexBufferLayouts.count(VL_PN) == 0 && slot == VL_PN) {
        std::vector<WGPUVertexAttribute>& wgpVertexAttribute = wgpVertexAttributes[VL_PN];
        wgpVertexAttribute.resize(2);

        wgpVertexAttribute[0].shaderLocation = 0;
        wgpVertexAttribute[0].format = WGPUVertexFormat::WGPUVertexFormat_Float32x3;
        wgpVertexAttribute[0].offset = 0;

        wgpVertexAttribute[1].shaderLocation = 1;
        wgpVertexAttribute[1].format = WGPUVertexFormat::WGPUVertexFormat_Float32x3;
        wgpVertexAttribute[1].offset = 3 * sizeof(float);

        WGPUVertexBufferLayout wgpVertexBufferLayout = {};
        wgpVertexBufferLayout.attributeCount = (uint32_t)wgpVertexAttribute.size();
        wgpVertexBufferLayout.attributes = wgpVertexAttribute.data();
        wgpVertexBufferLayout.arrayStride = 6 * sizeof(float);
        wgpVertexBufferLayout.stepMode = WGPUVertexStepMode::WGPUVertexStepMode_Vertex;
        wgpVertexBufferLayouts[VL_PN].push_back(wgpVertexBufferLayout);
    }else if (wgpVertexBufferLayouts.count(VL_PNC) == 0 && slot == VL_PNC) {
        std::vector<WGPUVertexAttribute>& wgpVertexAttribute = wgpVertexAttributes[VL_PNC];
        wgpVertexAttribute.resize(3);

        wgpVertexAttribute[0].shaderLocation = 0;
        wgpVertexAttribute[0].format = WGPUVertexFormat::WGPUVertexFormat_Float32x3;
        wgpVertexAttribute[0].offset = 0;

        wgpVertexAttribute[1].shaderLocation = 1;
        wgpVertexAttribute[1].format = WGPUVertexFormat::WGPUVertexFormat_Float32x3;
        wgpVertexAttribute[1].offset = 3 * sizeof(float);

        wgpVertexAttribute[2].shaderLocation = 2;
        wgpVertexAttribute[2].format = WGPUVertexFormat::WGPUVertexFormat_Float32x4;
        wgpVertexAttribute[2].offset = 6 * sizeof(float);

        WGPUVertexBufferLayout wgpVertexBufferLayout = {};
        wgpVertexBufferLayout.attributeCount = (uint32_t)wgpVertexAttribute.size();
        wgpVertexBufferLayout.attributes = wgpVertexAttribute.data();
        wgpVertexBufferLayout.arrayStride = 10 * sizeof(float);
        wgpVertexBufferLayout.stepMode = WGPUVertexStepMode::WGPUVertexStepMode_Vertex;
        wgpVertexBufferLayouts[VL_PNC].push_back(wgpVertexBufferLayout);
    }else if (wgpVertexBufferLayouts.count(VL_PTN) == 0 && slot == VL_PTN) {
        std::vector<WGPUVertexAttribute>& wgpVertexAttribute = wgpVertexAttributes[VL_PTN];
        wgpVertexAttribute.resize(3);

        wgpVertexAttribute[0].shaderLocation = 0;
        wgpVertexAttribute[0].format = WGPUVertexFormat::WGPUVertexFormat_Float32x3;
        wgpVertexAttribute[0].offset = 0;

        wgpVertexAttribute[1].shaderLocation = 1;
        wgpVertexAttribute[1].format = WGPUVertexFormat::WGPUVertexFormat_Float32x2;
        wgpVertexAttribute[1].offset = 3 * sizeof(float);

        wgpVertexAttribute[2].shaderLocation = 2;
        wgpVertexAttribute[2].format = WGPUVertexFormat::WGPUVertexFormat_Float32x3;
        wgpVertexAttribute[2].offset = 5 * sizeof(float);

        WGPUVertexBufferLayout wgpVertexBufferLayout = {};
        wgpVertexBufferLayout.attributeCount = (uint32_t)wgpVertexAttribute.size();
        wgpVertexBufferLayout.attributes = wgpVertexAttribute.data();
        wgpVertexBufferLayout.arrayStride = 8 * sizeof(float);
        wgpVertexBufferLayout.stepMode = WGPUVertexStepMode::WGPUVertexStepMode_Vertex;
        wgpVertexBufferLayouts[VL_PTN].push_back(wgpVertexBufferLayout);
    }else if (wgpVertexBufferLayouts.count(VL_PTNC) == 0 && slot == VL_PTNC) {
        std::vector<WGPUVertexAttribute>& wgpVertexAttribute = wgpVertexAttributes[VL_PTNC];
        wgpVertexAttribute.resize(4);

        wgpVertexAttribute[0].shaderLocation = 0;
        wgpVertexAttribute[0].format = WGPUVertexFormat::WGPUVertexFormat_Float32x3;
        wgpVertexAttribute[0].offset = 0;

        wgpVertexAttribute[1].shaderLocation = 1;
        wgpVertexAttribute[1].format = WGPUVertexFormat::WGPUVertexFormat_Float32x2;
        wgpVertexAttribute[1].offset = 3 * sizeof(float);

        wgpVertexAttribute[2].shaderLocation = 2;
        wgpVertexAttribute[2].format = WGPUVertexFormat::WGPUVertexFormat_Float32x3;
        wgpVertexAttribute[2].offset = 5 * sizeof(float);

        wgpVertexAttribute[3].shaderLocation = 3;
        wgpVertexAttribute[3].format = WGPUVertexFormat::WGPUVertexFormat_Float32x4;
        wgpVertexAttribute[3].offset = 8 * sizeof(float);

        WGPUVertexBufferLayout wgpVertexBufferLayout = {};
        wgpVertexBufferLayout.attributeCount = (uint32_t)wgpVertexAttribute.size();
        wgpVertexBufferLayout.attributes = wgpVertexAttribute.data();
        wgpVertexBufferLayout.arrayStride = 12 * sizeof(float);
        wgpVertexBufferLayout.stepMode = WGPUVertexStepMode::WGPUVertexStepMode_Vertex;
        wgpVertexBufferLayouts[VL_PTNC].push_back(wgpVertexBufferLayout);
    }else if (wgpVertexBufferLayouts.count(VL_PTNTB) == 0 && slot == VL_PTNTB) {
        std::vector<WGPUVertexAttribute>& wgpVertexAttribute = wgpVertexAttributes[VL_PTNTB];
        wgpVertexAttribute.resize(5);

        wgpVertexAttribute[0].shaderLocation = 0;
        wgpVertexAttribute[0].format = WGPUVertexFormat::WGPUVertexFormat_Float32x3;
        wgpVertexAttribute[0].offset = 0;

        wgpVertexAttribute[1].shaderLocation = 1;
        wgpVertexAttribute[1].format = WGPUVertexFormat::WGPUVertexFormat_Float32x2;
        wgpVertexAttribute[1].offset = 3 * sizeof(float);

        wgpVertexAttribute[2].shaderLocation = 2;
        wgpVertexAttribute[2].format = WGPUVertexFormat::WGPUVertexFormat_Float32x3;
        wgpVertexAttribute[2].offset = 5 * sizeof(float);

        wgpVertexAttribute[3].shaderLocation = 3;
        wgpVertexAttribute[3].format = WGPUVertexFormat::WGPUVertexFormat_Float32x3;
        wgpVertexAttribute[3].offset = 8 * sizeof(float);

        wgpVertexAttribute[4].shaderLocation = 4;
        wgpVertexAttribute[4].format = WGPUVertexFormat::WGPUVertexFormat_Float32x3;
        wgpVertexAttribute[4].offset = 11 * sizeof(float);

        WGPUVertexBufferLayout wgpVertexBufferLayout = {};
        wgpVertexBufferLayout.attributeCount = (uint32_t)wgpVertexAttribute.size();
        wgpVertexBufferLayout.attributes = wgpVertexAttribute.data();
        wgpVertexBufferLayout.arrayStride = 14 * sizeof(float);
        wgpVertexBufferLayout.stepMode = WGPUVertexStepMode::WGPUVertexStepMode_Vertex;
        wgpVertexBufferLayouts[VL_PTNTB].push_back(wgpVertexBufferLayout);
    }else if (wgpVertexBufferLayouts.count(VL_PTNWJ) == 0 && slot == VL_PTNWJ) {
        std::vector<WGPUVertexAttribute>& wgpVertexAttribute = wgpVertexAttributes[VL_PTNWJ];
        wgpVertexAttribute.resize(5);

        wgpVertexAttribute[0].shaderLocation = 0;
        wgpVertexAttribute[0].format = WGPUVertexFormat::WGPUVertexFormat_Float32x3;
        wgpVertexAttribute[0].offset = 0;

        wgpVertexAttribute[1].shaderLocation = 1;
        wgpVertexAttribute[1].format = WGPUVertexFormat::WGPUVertexFormat_Float32x2;
        wgpVertexAttribute[1].offset = 3 * sizeof(float);

        wgpVertexAttribute[2].shaderLocation = 2;
        wgpVertexAttribute[2].format = WGPUVertexFormat::WGPUVertexFormat_Float32x3;
        wgpVertexAttribute[2].offset = 5 * sizeof(float);

        wgpVertexAttribute[3].shaderLocation = 3;
        wgpVertexAttribute[3].format = WGPUVertexFormat::WGPUVertexFormat_Float32x4;
        wgpVertexAttribute[3].offset = 8 * sizeof(float);

        wgpVertexAttribute[4].shaderLocation = 4;
        wgpVertexAttribute[4].format = WGPUVertexFormat::WGPUVertexFormat_Uint32x4;
        wgpVertexAttribute[4].offset = 12 * sizeof(float);

        WGPUVertexBufferLayout wgpVertexBufferLayout = {};
        wgpVertexBufferLayout.attributeCount = (uint32_t)wgpVertexAttribute.size();
        wgpVertexBufferLayout.attributes = wgpVertexAttribute.data();
        wgpVertexBufferLayout.arrayStride = 12 * sizeof(float) + 4 * sizeof(uint32_t);
        wgpVertexBufferLayout.stepMode = WGPUVertexStepMode::WGPUVertexStepMode_Vertex;
        wgpVertexBufferLayouts[VL_PTNWJ].push_back(wgpVertexBufferLayout);
    }else if (wgpVertexBufferLayouts.count(VL_BATCH) == 0 && slot == VL_BATCH) {
        std::vector<WGPUVertexAttribute>& wgpVertexAttribute = wgpVertexAttributes[VL_BATCH];
        wgpVertexAttribute.resize(4);

        wgpVertexAttribute[0].shaderLocation = 0u;
        wgpVertexAttribute[0].format = WGPUVertexFormat::WGPUVertexFormat_Float32x3;
        wgpVertexAttribute[0].offset = 0u;

        wgpVertexAttribute[1].shaderLocation = 1u;
        wgpVertexAttribute[1].format = WGPUVertexFormat::WGPUVertexFormat_Float32x2;
        wgpVertexAttribute[1].offset = 3 * sizeof(float);

        wgpVertexAttribute[2].shaderLocation = 2u;
        wgpVertexAttribute[2].format = WGPUVertexFormat::WGPUVertexFormat_Float32x4;
        wgpVertexAttribute[2].offset = 5 * sizeof(float);

        wgpVertexAttribute[3].shaderLocation = 3u;
        wgpVertexAttribute[3].format = WGPUVertexFormat::WGPUVertexFormat_Uint32;
        wgpVertexAttribute[3].offset = 9 * sizeof(float);

        WGPUVertexBufferLayout wgpVertexBufferLayout = {};
        wgpVertexBufferLayout.attributeCount = (uint32_t)wgpVertexAttribute.size();
        wgpVertexBufferLayout.attributes = wgpVertexAttribute.data();
        wgpVertexBufferLayout.arrayStride = 9 * sizeof(float) + sizeof(unsigned int);
        wgpVertexBufferLayout.stepMode = WGPUVertexStepMode::WGPUVertexStepMode_Vertex;
        wgpVertexBufferLayouts[VL_BATCH].push_back(wgpVertexBufferLayout);
    }
}

WGPUSampler wgpCreateSampler(WGPUFilterMode filterMode, WGPUAddressMode addressMode, uint16_t maxAnisotropy, WGPUMipmapFilterMode mipmapFilterMode, WGPUCompareFunction compareFunction) {
    const WGPUDevice& device = wgpContext.device;
    WGPUSamplerDescriptor samplerDescriptor = {};
    samplerDescriptor.label = WGPU_STR("sampler");
    samplerDescriptor.addressModeU = addressMode;
    samplerDescriptor.addressModeV = addressMode;
    samplerDescriptor.addressModeW = addressMode;
    samplerDescriptor.magFilter = filterMode;
    samplerDescriptor.minFilter = filterMode;
    samplerDescriptor.mipmapFilter = mipmapFilterMode == WGPUMipmapFilterMode_Undefined ? ((filterMode == WGPUFilterMode_Nearest) ? WGPUMipmapFilterMode_Nearest : WGPUMipmapFilterMode_Linear) : mipmapFilterMode;
    samplerDescriptor.lodMinClamp = 0.0f;
    samplerDescriptor.lodMaxClamp = 1.0f;
    samplerDescriptor.compare = compareFunction;
    samplerDescriptor.maxAnisotropy = maxAnisotropy;
    return wgpuDeviceCreateSampler(device, &samplerDescriptor);
}

void wgpShutDown() {
    wgpPipelineLayoutsRelease();
    wgpPipelinesRelease();
    wgpSamplersRelease();
    wgpShaderModulesRelease();

    wgpuTextureViewRelease(wgpContext.depthTextureView);
    wgpContext.depthTextureView = nullptr;

    wgpuTextureDestroy(wgpContext.depthTexture);
    wgpuTextureRelease(wgpContext.depthTexture);
    wgpContext.depthTexture = nullptr;

    if (wgpContext.msaaTextureView) {
        wgpuTextureViewRelease(wgpContext.msaaTextureView);
        wgpContext.msaaTextureView = nullptr;
    }

    if (wgpContext.msaaTexture) {
        wgpuTextureDestroy(wgpContext.msaaTexture);
        wgpuTextureRelease(wgpContext.msaaTexture);
        wgpContext.msaaTexture = nullptr;
    }

    wgpuQueueRelease(wgpContext.queue);
    wgpContext.queue = nullptr;

    wgpuAdapterRelease(wgpContext.adapter);
    wgpContext.adapter = nullptr;

    wgpuSurfaceRelease(wgpContext.surface);
    wgpContext.surface = nullptr;

    wgpuInstanceRelease(wgpContext.instance);
    wgpContext.instance = nullptr;

    wgpuDeviceDestroy(wgpContext.device);
    wgpuDeviceRelease(wgpContext.device);
    wgpContext.device = nullptr;
}

void wgpSamplersRelease() {
    for (auto& it : wgpContext.samplers) {
        wgpuSamplerRelease(it.second);
    }

    wgpContext.samplers.clear();
    wgpContext.samplers.rehash(0u);
}

void wgpShaderModulesRelease() {
    for (auto& it : wgpContext.shaderModules) {
        wgpuShaderModuleRelease(it.second);
    }

    wgpContext.shaderModules.clear();
    wgpContext.shaderModules.rehash(0u);
}

void wgpPipelinesRelease() {
    for (auto& it : wgpContext.renderPipelines) {
        WGPUBindGroupLayout bindGroupLayout = wgpuRenderPipelineGetBindGroupLayout(it.second, 0);
        wgpuBindGroupLayoutRelease(bindGroupLayout);
        wgpuRenderPipelineRelease(it.second);
    }

    wgpContext.renderPipelines.clear();
    wgpContext.renderPipelines.rehash(0u);

    for (auto& it : wgpContext.computePipelines) {
        WGPUBindGroupLayout bindGroupLayout = wgpuComputePipelineGetBindGroupLayout(it.second, 0);
        wgpuBindGroupLayoutRelease(bindGroupLayout);
        wgpuComputePipelineRelease(it.second);
    }

    wgpContext.computePipelines.clear();
    wgpContext.computePipelines.rehash(0u);
}

void wgpPipelineLayoutsRelease() {
    for (auto& it : wgpContext.pipelineLayouts) {
        wgpuPipelineLayoutRelease(it.second);
    }

    wgpContext.pipelineLayouts.clear();
    wgpContext.pipelineLayouts.rehash(0u);
}

void wgpToggleVerticalSync() {
    if (wgpContext.surface) {
        wgpContext.config.presentMode = wgpContext.config.presentMode == WGPUPresentMode_Fifo ? WGPUPresentMode_Immediate : WGPUPresentMode_Fifo;
        wgpuSurfaceConfigure(wgpContext.surface, &wgpContext.config);
    }
}

void wgpCreateSurface(void* window){
    WGPUSurfaceSourceAndroidNativeWindow androidSource = {};
    androidSource.chain.sType = WGPUSType_SurfaceSourceAndroidNativeWindow;
    androidSource.chain.next = nullptr;
    androidSource.window = window;

    WGPUSurfaceDescriptor surfaceDescriptor = {};
    surfaceDescriptor.nextInChain = &androidSource.chain;

    if(!wgpContext.surface) {
        wgpContext.surface = wgpuInstanceCreateSurface(wgpContext.instance, &surfaceDescriptor);
        wgpContext.surfaceCapabilities = {};
        wgpuSurfaceGetCapabilities(wgpContext.surface, wgpContext.adapter, &wgpContext.surfaceCapabilities);
    }else{
        wgpuSurfaceRelease(wgpContext.surface);
        wgpContext.surface = wgpuInstanceCreateSurface(wgpContext.instance, &surfaceDescriptor);;
    }
}

void wgpSetSurfaceColorFormat(WGPUTextureFormat textureFormat, const std::function<void()>& onSurfaceChange) {
    if (wgpContext.surface) {
        wgpContext.colorFormat = textureFormat;
        wgpContext.config.format = wgpContext.colorFormat;
        wgpuSurfaceConfigure(wgpContext.surface, &wgpContext.config);

        if (wgpContext.msaaSampleCount > 1u) {
            uint32_t width = wgpuTextureGetWidth(wgpContext.msaaTexture);
            uint32_t height = wgpuTextureGetHeight(wgpContext.msaaTexture);

            wgpuTextureViewRelease(wgpContext.msaaTextureView);
            wgpuTextureDestroy(wgpContext.msaaTexture);
            wgpuTextureRelease(wgpContext.msaaTexture);

            wgpContext.msaaTexture = wgpCreateTexture(width, height, 1u, WGPUTextureUsage_RenderAttachment, wgpContext.colorFormat, 1u, wgpContext.msaaSampleCount, wgpContext.colorFormat);
            wgpContext.msaaTextureView = wgpCreateTextureView(wgpContext.msaaTexture, WGPUTextureAspect::WGPUTextureAspect_All);
        }
        if (onSurfaceChange)
            onSurfaceChange();
    }
}

void wgpSetSurfaceDepthFormat(WGPUTextureFormat textureFormat, const std::function<void()>& onSurfaceChange) {
    if (wgpContext.surface) {
        wgpContext.depthFormat = textureFormat;

        uint32_t width = wgpuTextureGetWidth(wgpContext.depthTexture);
        uint32_t height = wgpuTextureGetHeight(wgpContext.depthTexture);

        wgpuTextureViewRelease(wgpContext.depthTextureView);
        wgpuTextureDestroy(wgpContext.depthTexture);
        wgpuTextureRelease(wgpContext.depthTexture);

        wgpContext.depthTexture = wgpCreateTexture(width, height, 1u, WGPUTextureUsage_RenderAttachment, wgpContext.depthFormat, 1u, wgpContext.msaaSampleCount, wgpContext.depthFormat);
        wgpContext.depthTextureView = wgpCreateTextureView(wgpContext.depthTexture, WGPUTextureAspect::WGPUTextureAspect_All);

        if (onSurfaceChange)
            onSurfaceChange();

    }
}

void wgpSetMSAASampleCount(const uint32_t count, const std::function<void()>& onSurfaceChange) {
    if(wgpContext.msaaSampleCount != count){
        wgpContext.msaaSampleCount = count;

        uint32_t width = wgpuTextureGetWidth(wgpContext.depthTexture);
        uint32_t height = wgpuTextureGetHeight(wgpContext.depthTexture);

        if (wgpContext.msaaTexture) {
            wgpuTextureViewRelease(wgpContext.msaaTextureView);
            wgpuTextureDestroy(wgpContext.msaaTexture);
            wgpuTextureRelease(wgpContext.msaaTexture);
        }

        wgpContext.msaaTexture = count == 1u ? nullptr : wgpCreateTexture(width, height, 1u, WGPUTextureUsage_RenderAttachment, wgpContext.colorFormat, 1u, wgpContext.msaaSampleCount, wgpContext.colorFormat);
        wgpContext.msaaTextureView = count == 1u ? nullptr : wgpCreateTextureView(wgpContext.msaaTexture, WGPUTextureAspect::WGPUTextureAspect_All);

        wgpuTextureViewRelease(wgpContext.depthTextureView);
        wgpuTextureDestroy(wgpContext.depthTexture);
        wgpuTextureRelease(wgpContext.depthTexture);

        wgpContext.depthTexture = wgpCreateTexture(width, height, 1u, WGPUTextureUsage_RenderAttachment, wgpContext.depthFormat, 1u, wgpContext.msaaSampleCount, wgpContext.depthFormat);
        wgpContext.depthTextureView = wgpCreateTextureView(wgpContext.depthTexture, WGPUTextureAspect::WGPUTextureAspect_All);

        if (onSurfaceChange)
            onSurfaceChange();
    }
}

WGPURenderPassDepthStencilAttachment wgpCopyDepthStencilAttachment(const WGPURenderPassDepthStencilAttachment* src) {
    WGPURenderPassDepthStencilAttachment dest;
    dest.depthClearValue = src->depthClearValue;
    dest.depthLoadOp = src->depthLoadOp;
    dest.depthReadOnly = src->depthReadOnly;
    dest.depthStoreOp = src->depthStoreOp;

    dest.stencilClearValue = src->stencilClearValue;
    dest.stencilLoadOp = src->stencilLoadOp;
    dest.stencilReadOnly = src->stencilReadOnly;
    dest.stencilStoreOp = src->stencilStoreOp;

    dest.nextInChain = src->nextInChain;
    dest.view = src->view;

    return dest;
}


void wgpDraw() {
    WGPUSurfaceTexture surfaceTexture;
    wgpuSurfaceGetCurrentTexture(wgpContext.surface, &surfaceTexture);
    switch (surfaceTexture.status) {
        case WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal:
        case WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal:
        case WGPUSurfaceGetCurrentTextureStatus_Error:
            break;
        case WGPUSurfaceGetCurrentTextureStatus_Timeout:
        case WGPUSurfaceGetCurrentTextureStatus_Outdated:
        case WGPUSurfaceGetCurrentTextureStatus_Lost: {
            if (surfaceTexture.texture != nullptr) {
                wgpuTextureRelease(surfaceTexture.texture);
                wgpuSurfaceConfigure(wgpContext.surface, &wgpContext.config);
            }
            return;
        }
    }

    WGPUTextureView texureView = wgpuTextureCreateView(surfaceTexture.texture, nullptr);
    WGPURenderPassColorAttachment renderPassColorAttachment = {};
    renderPassColorAttachment.view = wgpContext.msaaSampleCount == 1u ? texureView  : wgpContext.msaaTextureView;
    renderPassColorAttachment.resolveTarget = wgpContext.msaaSampleCount == 1u ? nullptr : texureView;
    renderPassColorAttachment.loadOp = WGPULoadOp::WGPULoadOp_Clear;
    renderPassColorAttachment.storeOp = WGPUStoreOp::WGPUStoreOp_Store;
    renderPassColorAttachment.clearValue = wgpContext.clearColor;
    renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

    WGPURenderPassDepthStencilAttachment depthStencilAttachment = {};
    depthStencilAttachment.view = wgpContext.depthTextureView;
    depthStencilAttachment.depthClearValue = 1.0f;
    depthStencilAttachment.depthLoadOp = WGPULoadOp::WGPULoadOp_Clear;
    depthStencilAttachment.depthStoreOp = WGPUStoreOp::WGPUStoreOp_Store;
    depthStencilAttachment.depthReadOnly = WGPUOptionalBool::WGPUOptionalBool_False;
    depthStencilAttachment.stencilClearValue = 0u;
    depthStencilAttachment.stencilLoadOp = WGPULoadOp::WGPULoadOp_Undefined;
    depthStencilAttachment.stencilStoreOp = WGPUStoreOp::WGPUStoreOp_Undefined;
    depthStencilAttachment.stencilReadOnly = WGPUOptionalBool::WGPUOptionalBool_True;

    WGPURenderPassDescriptor renderPassDescriptor = {};
    renderPassDescriptor.colorAttachmentCount = 1u;
    renderPassDescriptor.colorAttachments = &renderPassColorAttachment;
    renderPassDescriptor.depthStencilAttachment = &depthStencilAttachment;
    renderPassDescriptor.timestampWrites = nullptr;

    WGPUCommandEncoderDescriptor commandEncoderDescriptor = {};
    commandEncoderDescriptor.label = WGPU_STR("command_encoder");
    wgpContext.commandEncoder = wgpuDeviceCreateCommandEncoder(wgpContext.device, &commandEncoderDescriptor);

    wgpContext.OnDraw(wgpContext.commandEncoder, renderPassDescriptor);

    wgpuTextureViewRelease(texureView);

    WGPUCommandBufferDescriptor commandBufferDescriptor = {};
    commandBufferDescriptor.label = WGPU_STR("command_buffer");
    WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(wgpContext.commandEncoder, &commandBufferDescriptor);

    wgpuQueueSubmit(wgpContext.queue, 1, &commandBuffer);

    wgpuSurfacePresent(wgpContext.surface);

    wgpuCommandBufferRelease(commandBuffer);
    wgpuCommandEncoderRelease(wgpContext.commandEncoder);
    wgpuTextureRelease(surfaceTexture.texture);

    if (wgpContext.OnPostDraw)
        wgpContext.OnPostDraw();

    wgpuInstanceProcessEvents(wgpContext.instance);
}

void wgpSubmitQueue() {
    WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(wgpContext.commandEncoder, nullptr);
    wgpuQueueSubmit(wgpContext.queue, 1, &commandBuffer);
    wgpuCommandBufferRelease(commandBuffer);
    wgpuCommandEncoderRelease(wgpContext.commandEncoder);

    wgpContext.commandEncoder = wgpuDeviceCreateCommandEncoder(wgpContext.device, nullptr);
}

void WgpContext::addSampler(const WGPUSampler& sampler, SamplerSlot samplerSlot) {
    if (samplers.count(samplerSlot) == 0)
        samplers[samplerSlot] = sampler;
}

const WGPUSampler& WgpContext::getSampler(SamplerSlot samplerSlot) const {
    return samplers.at(samplerSlot);
}

void WgpContext::addSahderModule(const std::string& shaderModuleName, const std::string& stringPath, bool fromString) {
    shaderModules[shaderModuleName] = fromString ? wgpCreateShaderFromString(stringPath) : wgpCreateShaderFromFile(stringPath);
}

const WGPUShaderModule& WgpContext::getShaderModule(std::string shaderModuleName) const {
    return shaderModules.at(shaderModuleName);
}

const WGPUPipelineLayout& WgpContext::getPipelineLayout(std::string pipelineLayoutName) const {
    return pipelineLayouts.at(pipelineLayoutName);
}

void WgpContext::setClearColor(const WGPUColor& _clearColor) {
    clearColor = _clearColor;
}

bool WgpContext::isBlendAble(WGPUTextureFormat textureFormat) {
    return textureFormat != WGPUTextureFormat_R32Uint && textureFormat != WGPUTextureFormat_R32Sint;
}