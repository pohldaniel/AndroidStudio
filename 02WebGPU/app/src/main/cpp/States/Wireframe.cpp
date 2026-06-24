#include "WgpContext.h"
#include "Wireframe.h"

Wireframe::Wireframe(StateMachine& machine) : State(machine, States::WIREFRAME) {
    wgpContext.OnDraw = std::bind(&Wireframe::OnDraw, this, std::placeholders::_1, std::placeholders::_2);
}

Wireframe::~Wireframe() {

}

void Wireframe::OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor) {
    WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(wgpContext.commandEncoder, &renderPassDescriptor);
    wgpuRenderPassEncoderEnd(renderPassEncoder);
    wgpuRenderPassEncoderRelease(renderPassEncoder);
}

void Wireframe::fixedUpdate() {

}

void Wireframe::update() {

}

void Wireframe::render() {

}