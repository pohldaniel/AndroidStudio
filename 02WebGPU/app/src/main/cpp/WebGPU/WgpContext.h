#include "webgpu.h"
    #include "wgpu.h"

#define WGPU_STR(str) { str, sizeof(str) - 1 }
#define STRVIEW(str) WGPUStringView{ str, sizeof(str) - 1 }

struct WgpContext;
extern WgpContext wgpContext;

extern "C" {
	void wgpInit(void* window);
	void wgpCreateDevice(void* window);
}

struct WgpContext {
    WGPUInstance instance = NULL;
};