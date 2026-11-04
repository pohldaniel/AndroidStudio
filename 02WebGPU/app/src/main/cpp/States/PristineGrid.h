#pragma once

#include <webgpu.h>

#include <WebGPU/WgpData.h>
#include <WebGPU/WgpTexture.h>
#include <WebGPU/WgpBuffer.h>
#include <WebGPU/WgpModel.h>

#include <States/StateMachine.h>
#include <Shape/Shape.h>

#include <core/Camera.h>

class PristineGrid : public State {
    struct GridOption {
        glm::vec4 line_color;
        glm::vec4 base_color;
        glm::vec2 line_width;
        glm::vec2 padding;
    };
public:

    PristineGrid(StateMachine& machine);
    ~PristineGrid() override;

    void fixedUpdate() override;
    void update() override;
    void render() override;
    void OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor);

    void OnButton(const Event::MouseButtonEvent& event) override;
    void resize(int deltaW, int deltaH) override;

private:

    std::vector<WGPUBindGroupLayout> OnBindGroupLayouts();
    std::vector<WGPUBindGroup> OnBindGroups();

    void renderUi(const WGPURenderPassEncoder& renderPassEncoder);
    bool m_initUi = true;
    bool m_drawUi = true;

    Camera m_camera;
    Uniforms m_uniforms;
    GridOption m_gridOption;
    Shape m_quad;
    glm::vec4 m_clearColor;
    WgpBuffer m_uniformBuffer, m_gridOptionBuffer;
    WgpModel m_wgpQuad;
};