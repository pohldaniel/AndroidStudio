#pragma once

#include <webgpu.h>

#include <WebGpu/WgpData.h>
#include <WebGPU/WgpTexture.h>
#include <WebGPU/WgpBuffer.h>
#include <WebGPU/WgpModel.h>

#include <Shape/Shape.h>
#include <States/StateMachine.h>

#include <core/Camera.h>
#include <core/AssimpModel.h>
#include <core/ObjModel.h>
#include <core/TrackBall.h>

class Wireframe : public State {
    enum SelectedModel {
        MAMMOTH,
        DRAGON
    };
public:
    Wireframe(StateMachine& machine);
    ~Wireframe() override;

    void fixedUpdate() override;
    void update() override;
    void render() override;
    void resize(int deltaW, int deltaH) override;
    void OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor);
    void OnButton() override;

private:

    std::vector<WGPUBindGroupLayout> OnBindGroupLayouts();
    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsWF();

    std::vector<WGPUBindGroup> OnBindGroups();
    std::vector<WGPUBindGroup> OnBindGroupsWF();

    Camera m_camera;
    ObjModel m_dragon;
    TrackBall m_trackball;
    Uniforms m_uniforms;

    WgpBuffer m_wgpBuffer;
    WgpModel m_wgpDragon;

    static void AddBindGroups(const WgpModel& model);
};