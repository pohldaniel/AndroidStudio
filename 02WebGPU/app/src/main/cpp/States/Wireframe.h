#pragma once

#include <webgpu.h>
#include "StateMachine.h"

class Wireframe : public State {

public:
    Wireframe(StateMachine& machine);
    ~Wireframe();

    void fixedUpdate() override;
    void update() override;
    void render() override;
    void OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor);
};