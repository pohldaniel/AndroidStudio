#include <WebGPU/WgpContext.h>

#include <States/Collada.h>
#include <States/Isometric.h>

#include "Globals.h"
#include "InputTouch.h"
#include "GamePad.h"
#include "Controller.h"

Controller::Controller(StateMachine& machine) : State(machine, States::CONTROLLER){
    m_camera.perspective(45.0f, static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 1000.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f , -1.0f, 1.0f);
    m_camera.lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_camera.setMovingSpeed(10.0f);
    m_camera.setRotationSpeed(0.1f);

    m_storageBuffer.createBuffer(20u * sizeof(float) * 200u, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage);

    m_uniformBuffer.createBuffer(sizeof(Uniforms), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);
    m_uniforms.projection = m_camera.getOrthographicMatrix();
    m_uniforms.view = m_camera.getViewMatrix();
    m_uniforms.env = m_camera.getRotationMatrix();
    m_uniforms.model = glm::mat4(1.0f);
    m_uniforms.normal = Camera::GetNormalMatrix(m_camera.getViewMatrix() * m_uniforms.model);
    m_uniforms.color = { 1.0f, 0.0f, 0.0f, 1.0f };
    m_uniforms.camPosition = m_camera.getPosition();
    m_uniforms.lightVP = glm::mat4(1.0f);
    m_uniforms.shadow = Camera::BIAS *  m_uniforms.lightVP;
    m_uniforms.lightPosition = glm::vec3(50.0f, 100.0f, -100.0f);
    wgpuQueueWriteBuffer(wgpContext.queue, m_uniformBuffer.getBuffer(), 0, &m_uniforms, sizeof(Uniforms));

    wgpContext.addSahderModule("COLOR_PT", "shader/controller.wgsl");
    wgpContext.createRenderPipeline("COLOR_PT", "RP_COLOR_PT", VL_PT, std::bind(&Controller::OnBindGroupLayouts, this));

    m_disk.buildSegmentXY({ 0.0f, 0.0f, 0.0f }, 20.0f, 0.0f, 360.0f, 1u, 30u, true, false);
    m_disk.rewind();

    m_quad.buildQuadXY({ -100.0f, -100.0f, 0.0f }, { 200.0f, 200.0f }, 1u, 1u, true, false);
    m_quad.rewind();

    m_wgpDisk.create(m_disk);
    m_wgpDisk.setBindGroups("BG", std::bind(&Controller::OnBindGroups, this));

    m_wgpQuad.create(m_quad);
    m_wgpQuad.setBindGroups("BG", std::bind(&Controller::OnBindGroups, this));

    wgpContext.setClearColor({ 0.494f, 0.686f, 0.796f, 1.0f });
    wgpContext.OnDraw = std::bind(&Controller::OnDraw, this, std::placeholders::_1, std::placeholders::_2);
}

Controller::~Controller() {
    m_uniformBuffer.markForDelete();
    m_storageBuffer.markForDelete();
}

void Controller::fixedUpdate() {

}

void Controller::update() {
    IGamePad* player = AndroidGamepad::GetPlayer();

    m_state = 0;
    if (player) {
        m_RightStickX = player->getRightStickX();
        m_RightStickY = player->getRightStickY();

        m_LeftStickX = player->getLeftStickX();
        m_LeftStickY = player->getLeftStickY();

        if (player->buttonDown(GamepadButton::DPAD_UP)) {
            m_state = 1;
        }

        if (player->buttonDown(GamepadButton::DPAD_RIGHT)) {
            m_state = 3;
        }

        if (player->buttonDown(GamepadButton::DPAD_DOWN)) {
            m_state = 5;
        }

        if (player->buttonDown(GamepadButton::DPAD_LEFT)) {
            m_state = 7;
        }

        if (player->buttonDown(GamepadButton::DPAD_UP_RIGHT)) {
            m_state = 2;
        }

        if (player->buttonDown(GamepadButton::DPAD_DOWN_RIGHT)) {
            m_state = 4;
        }

        if (player->buttonDown(GamepadButton::DPAD_DOWN_LEFT)) {
            m_state = 6;
        }

        if (player->buttonDown(GamepadButton::DPAD_UP_LEFT)) {
            m_state = 8;
        }

        if (player->buttonDown(GamepadButton::GAMEPAD_START)) {
            m_state = 9;
        }

        if (player->buttonDown(GamepadButton::GAMEPAD_BACK)) {
            m_state = 10;
        }

        if (player->buttonDown(GamepadButton::GAMEPAD_LEFT_THUMB)) {
            m_state = 11;
        }

        if (player->buttonDown(GamepadButton::GAMEPAD_RIGHT_THUMB)) {
            m_state = 12;
        }

        if (player->buttonDown(GamepadButton::GAMEPAD_LEFT_SHOULDER)) {
            m_state = 13;
        }

        if (player->buttonDown(GamepadButton::GAMEPAD_RIGHT_SHOULDER)) {
            m_state = 14;
        }

        if (player->buttonDown(GamepadButton::GAMEPAD_A)) {
            m_state = 15;
            if (player->buttonDown(GamepadButton::GAMEPAD_A)){
                player->setVibration(1.0f, 1.0f);
            }
        }

        if (player->buttonDown(GamepadButton::GAMEPAD_B)) {
            m_state = 16;
            if (player->buttonDown(GamepadButton::GAMEPAD_B)){
                player->setVibration(0.0f, 0.0f);
            }
        }

        if (player->buttonDown(GamepadButton::GAMEPAD_X)) {
            m_state = 17;
        }

        if (player->buttonDown(GamepadButton::GAMEPAD_Y)) {
            m_state = 18;
        }

        if (player->buttonDown(GamepadButton::GAMEPAD_LEFT_TRIGGER)) {
            m_state = 19;
        }

        if (player->buttonDown(GamepadButton::GAMEPAD_RIGHT_TRIGGER)) {
            m_state = 20;
        }
    }

    m_uniforms.projection = m_camera.getOrthographicMatrix();
    m_uniforms.view = m_camera.getViewMatrix();
    m_uniforms.env = m_camera.getRotationMatrix();
    m_uniforms.model = glm::mat4(1.0f);
    m_uniforms.normal = Camera::GetNormalMatrix(m_camera.getViewMatrix() * m_uniforms.model);
    m_uniforms.color = { 1.0f, 0.0f, 0.0f, 1.0f };
    m_uniforms.camPosition = m_camera.getPosition();
    m_uniforms.lightVP = glm::mat4(1.0f);
    m_uniforms.shadow = Camera::BIAS *  m_uniforms.lightVP;
    m_uniforms.lightPosition = glm::vec3(50.0f, 100.0f, -100.0f);
    wgpuQueueWriteBuffer(wgpContext.queue, m_uniformBuffer.getBuffer(), 0u, &m_uniforms, sizeof(Uniforms));

    m_instances.clear();

    renderRectangle(static_cast<float>(wgpWidth) * 0.5f + 300.0f, static_cast<float>(wgpHeight) * 0.5f);
    renderRectangle(static_cast<float>(wgpWidth) * 0.5f - 300.0f, static_cast<float>(wgpHeight) * 0.5f);

    renderPad(static_cast<float>(wgpWidth) * 0.5f - 80.0f, static_cast<float>(wgpHeight) * 0.5f + 80.0f, m_state);
    for (int i = 0; i < 12; i++) {
        renderButton(i * 50.0f + 50.0f, 300.0f, m_state, i + 9);
    }

    renderStick(static_cast<float>(wgpWidth) * 0.5f - 300.0f, static_cast<float>(wgpHeight) * 0.5f, m_RightStickX, m_RightStickY);
    renderStick(static_cast<float>(wgpWidth) * 0.5f + 300.0f, static_cast<float>(wgpHeight) * 0.5f, m_LeftStickX , m_LeftStickY);
    wgpuQueueWriteBuffer(wgpContext.queue, m_storageBuffer.getBuffer(), 0u, m_instances.data(), m_instances.size() * sizeof(float) * 20u);
}

void Controller::render() {
    wgpDraw();
}

void Controller::OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor) {
    {
        WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);
        wgpuRenderPassEncoderSetViewport(renderPassEncoder, 0.0f, 0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f, 1.0f);
        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_COLOR_PT"));

        m_wgpQuad.draw(renderPassEncoder, 4u, 0u);
        m_wgpDisk.draw(renderPassEncoder, m_instances.size() - 4u, 4u);

        wgpuRenderPassEncoderEnd(renderPassEncoder);
        wgpuRenderPassEncoderRelease(renderPassEncoder);
    }
}

void Controller::resize(int deltaW, int deltaH) {
    m_camera.perspective(glm::radians(45.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 1000.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f , -1.0f, 1.0f);
}

void Controller::OnButton(const Event::MouseButtonEvent& event) {
    wgpCleanState();
    m_isRunning = false;

    if(event.button == Event::MouseButtonEvent::BUTTON_LEFT){
        m_machine.addStateAtBottom(new Isometric(m_machine));
    }

    if(event.button == Event::MouseButtonEvent::BUTTON_RIGHT){
        m_machine.addStateAtBottom(new Collada(m_machine));
    }
}

std::vector<WGPUBindGroupLayout> Controller::OnBindGroupLayouts() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(2);
    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[0].buffer.minBindingSize = sizeof(Uniforms);

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[1].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_ReadOnlyStorage;
    bindingLayoutEntries[1].buffer.minBindingSize = 20u * sizeof(float);

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

std::vector<WGPUBindGroup> Controller::OnBindGroups() {
    std::vector<WGPUBindGroup> bindGroups(1);

    std::vector<WGPUBindGroupEntry> bindGroupEntries(2);
    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].buffer = m_uniformBuffer.getBuffer();
    bindGroupEntries[0].offset = 0u;
    bindGroupEntries[0].size = sizeof(Uniforms);

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].buffer = m_storageBuffer.getBuffer();
    bindGroupEntries[1].offset = 0u;
    bindGroupEntries[1].size = wgpuBufferGetSize(m_storageBuffer.getBuffer());

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_COLOR_PT"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();
    bindGroups[0] = wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);

    return bindGroups;
}

void Controller::renderRectangle(float x, float y) {
    std::array<float, 20> instance;
    instance[0] = 1.03f; instance[1] = 0.0f; instance[2] = 0.0f; instance[3] = 0.0f;
    instance[4] = 0.0f; instance[5] = 1.03f; instance[6] = 0.0f; instance[7] = 0.0f;
    instance[8] = 0.0f; instance[9] = 0.0f; instance[10] = 1.03f; instance[11] = 0.0f;
    instance[12] = x; instance[13] = y; instance[14] = 0.0f; instance[15] = 1.0f;
    instance[16] = 0.0f; instance[17] = 0.0f; instance[18] = 0.0f; instance[19] = 1.0f;
    m_instances.push_back(instance);

    instance[0] = 1.0f; instance[1] = 0.0f; instance[2] = 0.0f; instance[3] = 0.0f;
    instance[4] = 0.0f; instance[5] = 1.0f; instance[6] = 0.0f; instance[7] = 0.0f;
    instance[8] = 0.0f; instance[9] = 0.0f; instance[10] = 1.0f; instance[11] = 0.0f;
    instance[12] = x; instance[13] = y; instance[14] = 0.0f; instance[15] = 1.0f;
    instance[16] = 1.0f; instance[17] = 1.0f; instance[18] = 1.0f; instance[19] = 1.0f;
    m_instances.push_back(instance);
}

void Controller::renderPad(float x, float y, long value) {
    for (int i = 1; i < 9; i++) {
        float xPos = (sinf(glm::pi<float>()/4.0f * i - glm::pi<float>()/4.0f) * 80.0f) + 80.0f;
        float yPos = (cosf(glm::pi<float>()/4.0f * i - glm::pi<float>()/4.0f) * 80.0f) + 80.0f;

        std::array<float, 20> instance;
        instance[0] = 1.15f; instance[1] = 0.0f; instance[2] = 0.0f; instance[3] = 0.0f;
        instance[4] = 0.0f; instance[5] = 1.15f; instance[6] = 0.0f; instance[7] = 0.0f;
        instance[8] = 0.0f; instance[9] = 0.0f; instance[10] = 1.15f; instance[11] = 0.0f;
        instance[12] = x + xPos; instance[13] = y - yPos; instance[14] = 0.0f; instance[15] = 1.0f;
        instance[16] = 0.0f; instance[17] = 0.0f; instance[18] = 0.0f; instance[19] = 1.0f;
        m_instances.push_back(instance);

        instance[0] = 1.0f; instance[1] = 0.0f; instance[2] = 0.0f; instance[3] = 0.0f;
        instance[4] = 0.0f; instance[5] = 1.0f; instance[6] = 0.0f; instance[7] = 0.0f;
        instance[8] = 0.0f; instance[9] = 0.0f; instance[10] = 1.0f; instance[11] = 0.0f;
        instance[12] = x + xPos; instance[13] = y - yPos; instance[14] = 0.0f; instance[15] = 1.0f;
        instance[16] = 1.0f; instance[17] = value == i ? 0.0f : 1.0f; instance[18] = value == i ? 0.0f : 1.0f; instance[19] = 1.0f;
        m_instances.push_back(instance);
    }
}

void Controller::renderButton(float x, float y, long value, int index) {
    std::array<float, 20> instance;

    instance[0] = 1.15f; instance[1] = 0.0f; instance[2] = 0.0f; instance[3] = 0.0f;
    instance[4] = 0.0f; instance[5] = 1.15f; instance[6] = 0.0f; instance[7] = 0.0f;
    instance[8] = 0.0f; instance[9] = 0.0f; instance[10] = 1.15f; instance[11] = 0.0f;
    instance[12] = x; instance[13] = y; instance[14] = 0.0f; instance[15] = 1.0f;
    instance[16] = 0.0f; instance[17] = 0.0f; instance[18] = 0.0f; instance[19] = 1.0f;
    m_instances.push_back(instance);

    instance[0] = 1.0f; instance[1] = 0.0f; instance[2] = 0.0f; instance[3] = 0.0f;
    instance[4] = 0.0f; instance[5] = 1.0f; instance[6] = 0.0f; instance[7] = 0.0f;
    instance[8] = 0.0f; instance[9] = 0.0f; instance[10] = 1.0f; instance[11] = 0.0f;
    instance[12] = x; instance[13] = y; instance[14] = 0.0f; instance[15] = 1.0f;
    instance[16] = 1.0f; instance[17] = value == index ? 0.0f : 1.0f; instance[18] = value == index ? 0.0f : 1.0f; instance[19] = 1.0f;
    m_instances.push_back(instance);
}

void Controller::renderStick(float x, float y, float stickX, float stickY) {
    std::array<float, 20> instance;
    instance[0] = 1.0f; instance[1] = 0.0f; instance[2] = 0.0f; instance[3] = 0.0f;
    instance[4] = 0.0f; instance[5] = 1.0f; instance[6] = 0.0f; instance[7] = 0.0f;
    instance[8] = 0.0f; instance[9] = 0.0f; instance[10] = 1.0f; instance[11] = 0.0f;
    instance[12] = x + stickX * 50.0f; instance[13] = y - stickY * 50.0f; instance[14] = 0.0f; instance[15] = 1.0f;
    instance[16] = 1.0f; instance[17] = 0.0f; instance[18] = 0.0f; instance[19] = 1.0f;
    m_instances.push_back(instance);
}

void Controller::OnReEnter(unsigned int prevState) {
    update();
}