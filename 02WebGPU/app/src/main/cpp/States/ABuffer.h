#pragma once

#include <webgpu.h>

#include <WebGPU/WgpData.h>
#include <WebGPU/WgpTexture.h>
#include <WebGPU/WgpBuffer.h>
#include <WebGPU/WgpModel.h>

#include <States/StateMachine.h>
#include <Shape/Shape.h>

#include <core/Camera.h>
#include <core/TrackBall.h>
#include <core/ObjModel.h>

class ABuffer : public State {
    struct SliceInfo {
        uint32_t maxStorableFragments;
        uint32_t targetWidth;
        int32_t  sliceStartY;
        uint32_t padding;
    };
public:

    ABuffer(StateMachine& machine);
    ~ABuffer() override;

    void fixedUpdate() override;
    void update() override;
    void render() override;
    void OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor);

    void OnButton(const Event::MouseButtonEvent& event) override;
    void resize(int deltaW, int deltaH) override;

private:

    std::vector<WGPUBindGroupLayout> OnBindGroupLayouts();
    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsTranslucent();
    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsComposite();

    std::vector<WGPUBindGroup> OnBindGroups();
    WGPUBindGroup createBindGroupTranslucent();
    WGPUBindGroup createBindGroupComposite();

    void createBuffer();
    bool m_initUi = true;
    bool m_drawUi = true;

    Camera m_camera;
    Uniforms m_uniforms;
    TrackBall m_trackball;
    ObjModel m_teapot;

    WgpBuffer m_uniformBuffer, m_sliceInfoBuffer, m_linkedListBuffer, m_headsInitBuffer, m_headsBuffer;
    WgpModel m_wgpTeapot;
    WGPUBindGroup m_bindGroupTranslucent, m_bindGroupComposite;

    const uint32_t AverageLayers = 4u;

    uint32_t m_numSlices;
    uint32_t m_sliceHeight;

    static glm::vec3& RotateY(glm::vec3& p, float rad, const glm::vec3& centerOfRotation = glm::vec3(0.0f, 0.0f, 0.0f));
};