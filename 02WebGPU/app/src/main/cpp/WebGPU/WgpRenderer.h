#pragma once

#include <webgpu.h>

class WgpTexture;
class WgpBuffer;

struct WgpRenderer {
	static void DrawDepth(const WgpTexture& texture, const std::function<void(const WGPURenderPassEncoder& renderPassEncoder)>& OnDraw = nullptr);

	static void Draw(const WgpTexture& texture, const std::function<void(const WGPURenderPassEncoder& renderPassEncoder, uint32_t layer, uint32_t mip)>& OnDraw = nullptr);
	static void Dispatch(const WgpTexture& texture, const WgpBuffer& probability, const WgpBuffer& bufferA, const WgpBuffer& bufferB);
};