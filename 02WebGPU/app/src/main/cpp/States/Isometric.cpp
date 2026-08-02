#include <WebGPU/WgpContext.h>
#include <WebGPU/WgpRenderer.h>

#include <Nuklear/NkContext.h>
#include <Nuklear/NkStyle.h>

#include <States/VolumeRendering.h>
#include <States/Collada.h>

#include "InputTouch.h"
#include "Isometric.h"
#include "Logging.h"

glm::mat4 offset = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
                             0.0f, 1.0f, 0.0f, 0.0f,
                             0.0f, 0.0f, 1.0f, 0.0f,
                             -156.85f, -32.2427f, 144.702f, 1.0f);

glm::mat4 pivot = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
                            0.0f, 1.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 1.0f, 0.0f,
                            130.762f, 70.4033f, -3.52485f, 1.0f);

glm::mat4 invPivot = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
                               0.0f, 1.0f, 0.0f, 0.0f,
                               0.0f, 0.0f, 1.0f, 0.0f,
                               -130.762f, -70.4033f, 3.52485f, 1.0f);

Isometric::Isometric(StateMachine& machine) : State(machine, States::ISOMETRIC), m_animationController(&m_player) {

    nkInit(static_cast<float>(wgpWidth), static_cast<float>(wgpHeight));
    nkInitFont("fonts/upheavtt.ttf");

    m_camera.perspective(glm::radians(45.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 100.0f);
	m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f,  -1.0f, 1.0f);
    m_camera.lookAt(glm::vec3(-4.0f, 4.3f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	m_camera.setMovingSpeed(5.0f);
	m_camera.setRotationSpeed(0.1f);

    m_trackball.reshape(wgpWidth, wgpHeight);
    m_floor.buildQuadXZ({-50.0f, 0.0f, -50.0f}, {100.0f, 100.0f});

    AnimationManager::Get().getAnimation("full").loadAnimationAssimp("models/Player.fbx", "Player", "full", 0u, 245u);
    AnimationManager::Get().getAnimation("idle").loadAnimationAssimp("models/Player.fbx", "Player", "idle", 5u, 81u);
    AnimationManager::Get().getAnimation("forward").loadAnimationAssimp("models/Player.fbx", "Player", "forward", 85u, 105u);
    AnimationManager::Get().getAnimation("backward").loadAnimationAssimp("models/Player.fbx", "Player", "backward", 110u, 130u);
    AnimationManager::Get().getAnimation("backward").shift(10u);
    AnimationManager::Get().getAnimation("right").loadAnimationAssimp("models/Player.fbx", "Player", "right", 135u, 155u);
    AnimationManager::Get().getAnimation("right").shift(10u);
    AnimationManager::Get().getAnimation("left").loadAnimationAssimp("models/Player.fbx", "Player", "left", 160u, 180u);
    AnimationManager::Get().getAnimation("death").loadAnimationAssimp("models/Player.fbx", "Player", "death", 185u, 244u);

    m_player.loadModelAssimp("models/Player.fbx", 1u);

    AnimatedMesh* mesh = static_cast<AnimatedMesh*>(m_player.mesh());
    mesh->boneDescriptions().emplace_back();
    mesh->boneDescriptions().back().name = "Gun_$AssimpFbx$_Rotation";
    mesh->boneDescriptions().back().parentIndex = -1;
    mesh->boneDescriptions().back().offsetMatrix = invPivot;

    mesh->boneDescriptions().emplace_back();
    mesh->boneDescriptions().back().name = "Gun_$AssimpFbx$_Translation";
    mesh->boneDescriptions().back().parentIndex = 0;
    mesh->boneDescriptions().back().offsetMatrix = offset * pivot;

    mesh->createBones();

    mesh = static_cast<AnimatedMesh*>(m_player.mesh(1u));
    for (size_t index = 0u; index < mesh->getVertexBuffer().size() / mesh->getStride(); index++) {
        mesh->weights().push_back({ 1.0f, 0.0f, 0.0f, 0.0f });
        mesh->joints().push_back({ 42u, 0u, 0u, 0u });
    }

    m_player.scale(0.0044f, 0.0044f, 0.0044f);
    m_rotationButtonResult.degrees = 90.0f;

    m_uniformBuffer.createBuffer(sizeof(Uniforms), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);
    m_skinBuffer.createBuffer(sizeof(glm::mat4) * 96u, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage);

    m_uniforms.projection = m_camera.getPerspectiveMatrix();
    m_uniforms.view = m_camera.getViewMatrix();
    m_uniforms.env = m_camera.getRotationMatrix();
    m_uniforms.model = glm::mat4(1.0f);
    m_uniforms.normal = Camera::GetNormalMatrix(m_camera.getViewMatrix() * m_uniforms.model);
    m_uniforms.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    m_uniforms.camPosition = m_camera.getPosition();
    m_uniforms.lightVP = glm::mat4(1.0f);
    m_uniforms.shadow = Camera::BIAS *  m_uniforms.lightVP;
    m_uniforms.lightPosition = glm::vec3(50.0f, 100.0f, -100.0f);

    wgpuQueueWriteBuffer(wgpContext.queue, m_uniformBuffer.getBuffer(), 0, &m_uniforms, sizeof(Uniforms));

    wgpContext.addSahderModule("ANIMATION", "shader/animation_fbx.wgsl");
    wgpContext.createRenderPipeline("ANIMATION", "RP_ANIMATION", VL_PTNWJ, std::bind(&Isometric::OnBindGroupLayouts, this));

    wgpContext.addSahderModule("TEXTURE", "shader/texture.wgsl");
    wgpContext.createRenderPipeline("TEXTURE", "RP_TEXTURE", VL_PTN, std::bind(&Isometric::OnBindGroupLayoutsTexture, this));

    m_wgpPlayer.create(m_player);
    m_wgpPlayer.setBindGroups("BG", std::bind(&Isometric::OnBindGroups, this));

    m_wgpFloorD.loadFromFile("textures/floor/Floor_D.psd");

    m_wgpFloor.create(m_floor);
    m_wgpFloor.setBindGroups("BG", std::bind(&Isometric::OnBindGroupsTexture, this));

    wgpContext.setClearColor({ 0.2f, 0.2f, 0.2f, 1.0f });
    wgpContext.OnDraw = std::bind(&Isometric::OnDraw, this, std::placeholders::_1, std::placeholders::_2);
    nkContext.OnFillBuffer = std::bind(&Isometric::OnFillBuffer, this, std::placeholders::_1);

    m_animationController.play("idle", true, 0.2f);
    m_animationController.update(0.1f);
}

Isometric::~Isometric() {
    m_uniformBuffer.markForDelete();
    m_skinBuffer.markForDelete();
}

void Isometric::fixedUpdate() {

}

void Isometric::update() {
	nkUpdateInput(0, 0, false, false, 0.0f);
    m_trackball.idle();

    const glm::vec3 posistion = static_cast<const AnimatedMesh*>(m_player.getMesh())->getBone(0u).getPosition();
    m_camera.lookAt(posistion + glm::vec3(-4.0f, 4.3f, 0.0f), posistion, glm::vec3(0.0f, 1.0f, 0.0f));

    float degrees = m_rotationButtonResult.degrees;
    if(degrees != 0.0f && !m_isDeath)
        m_player.setRotation(0.0f, degrees, 0.0f);

    float moveX = 0.0f;
    float moveY = 0.0f;

    float magnitude = m_joystickResult.x * m_joystickResult.x + m_joystickResult.y * m_joystickResult.y;
    float deadzone = 0.25f;

    if (magnitude > deadzone * deadzone) {
        if (fabsf(m_joystickResult.x) > fabsf(m_joystickResult.y)) {
            moveX = (m_joystickResult.x > 0.0f) ? 1.0f : -1.0f;
            moveY = 0.0f;
        }else {
            moveX = 0.0f;
            moveY = (m_joystickResult.y > 0.0f) ? 1.0f : -1.0f;
        }
    }else {
        moveX = 0.0f;
        moveY = 0.0f;
    }

    bool playerMove = false;

    if (moveY > 0.0f && !m_isDeath) {
        playerMove |= true;
        m_animationController.fadeAndPlay("forward", 0.25f);
        m_player.translateRelative(0.0f, 0.0f, 2.0f * m_dt);
    }

    if (moveY < 0.0f && !m_isDeath) {
        playerMove |= true;
        m_animationController.fadeAndPlay("backward", 0.25f);
        m_player.translateRelative(0.0f, 0.0f, -2.0f * m_dt);
    }

    if (moveX < 0.0f && !m_isDeath) {
        playerMove |= true;
        m_animationController.fadeAndPlay("left", 0.25f);
        m_player.translateRelative(2.0f * m_dt, 0.0f, 0.0f);
    }

    if (moveX > 0.0f && !m_isDeath) {
        playerMove |= true;
        m_animationController.fadeAndPlay("right", 0.25f);
        m_player.translateRelative(-2.0f * m_dt, 0.0f, 0.0f);
    }

    if(m_rotationButtonResult.buttonPressed) {
        m_isDeath = true;
    }

    if (!playerMove && !m_isDeath) {
        m_animationController.fadeAndPlay("idle", 0.2f, 0.25f);
    }

    if (m_isDeath) {
        m_animationController.play("death", false, 2.0f);
    }

    m_animationController.update(m_dt);
    m_player.update(m_dt);
    m_player.updateSkinning();

    m_uniforms.projection = m_camera.getPerspectiveMatrix();
    m_uniforms.view = m_camera.getViewMatrix();
    m_uniforms.env = m_camera.getRotationMatrix();
    m_uniforms.model = glm::mat4(1.0f);
    m_uniforms.normal = Camera::GetNormalMatrix(m_camera.getViewMatrix() * m_uniforms.model);
    m_uniforms.camPosition = m_camera.getPosition();
    m_uniforms.lightVP = glm::mat4(1.0f);
    m_uniforms.shadow = Camera::BIAS * m_uniforms.lightVP;
    wgpuQueueWriteBuffer(wgpContext.queue, m_uniformBuffer.getBuffer(), 0, &m_uniforms, sizeof(Uniforms));

    const AnimatedMesh* mesh = static_cast<const AnimatedMesh*>(m_player.getMesh());
    mesh->skinMatrices()[42] = mesh->getBone(43u).getWorldTransformation() * offset * pivot * mesh->skinMatrices()[42];

    wgpuQueueWriteBuffer(wgpContext.queue, m_skinBuffer.getBuffer(), 0u, mesh->getSkinMatrices(), mesh->getNumBones() * sizeof(glm::mat4));
}

void Isometric::render() {
    wgpDraw();
}

void Isometric::OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor) {
    {
        WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);
        wgpuRenderPassEncoderSetViewport(renderPassEncoder, 0.0f, 0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f, 1.0f);

        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_TEXTURE"));
        m_wgpFloor.draw(renderPassEncoder);

        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_ANIMATION"));
        m_wgpPlayer.draw(renderPassEncoder);

        wgpuRenderPassEncoderEnd(renderPassEncoder);
        wgpuRenderPassEncoderRelease(renderPassEncoder);
    }

    {
        WGPURenderPassColorAttachment renderPassColorAttachment = renderPassDescriptor.colorAttachments[0];
        renderPassColorAttachment.loadOp = WGPULoadOp::WGPULoadOp_Load;

        WGPURenderPassDescriptor rndrPssDscrptor = renderPassDescriptor;
        rndrPssDscrptor.colorAttachments = &renderPassColorAttachment;

        nkDraw(commandEncoder, rndrPssDscrptor);
    }
}

void Isometric::OnFillBuffer(nk_context& nkCntxt) {

    int joystick_finger = -1;
    int action_finger = -1;
    for (int i = 0; i < MAX_TOUCH_POINTERS; i++) {
        if (touchStates[i].touchActive) {
            if (touchStates[i].touchX < (static_cast<float>(wgpWidth) / 2.0f)) {
                if (joystick_finger == -1) joystick_finger = i;
            } else {
                if (action_finger == -1) action_finger = i;
            }
        }
    }

    set_transparent_window_style();
    virtual_joystick(nk_rect(20.0f, static_cast<float>(wgpHeight) - 600.0f, 250.0f, 250.0f), joystick_finger, m_joystickResult);
    virtual_rotation_button(nk_rect(static_cast<float>(wgpWidth) - 250.0f,static_cast<float>(wgpHeight)  - 600.0f, 210.0f, 210.0f), action_finger, m_rotationButtonResult);
    reset_transparent_window_style();
}

void Isometric::resize(int deltaW, int deltaH) {
    nkResize(static_cast<float>(wgpWidth), static_cast<float>(wgpHeight));
    m_camera.perspective(glm::radians(45.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 100.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f, -1.0f, 1.0f);
    m_trackball.reshape(wgpWidth, wgpHeight);
}

void Isometric::OnButton(const Event::MouseButtonEvent& event) {
    wgpCleanState();
    nkShutDown();
    m_isRunning = false;

    if(event.button == Event::MouseButtonEvent::BUTTON_LEFT){
        m_machine.addStateAtBottom(new VolumeRendering(m_machine));
    }

    if(event.button == Event::MouseButtonEvent::BUTTON_RIGHT){
        m_machine.addStateAtBottom(new Collada(m_machine));
    }
}

std::vector<WGPUBindGroupLayout> Isometric::OnBindGroupLayouts() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(2);
    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[0].buffer.minBindingSize = sizeof(Uniforms);

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Vertex;
    bindingLayoutEntries[1].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_ReadOnlyStorage;
    bindingLayoutEntries[1].buffer.minBindingSize = 16 * sizeof(float);

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

std::vector<WGPUBindGroupLayout> Isometric::OnBindGroupLayoutsTexture() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(3);
    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[0].buffer.minBindingSize = sizeof(Uniforms);

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[1].sampler.type = WGPUSamplerBindingType::WGPUSamplerBindingType_Filtering;

    bindingLayoutEntries[2].binding = 2u;
    bindingLayoutEntries[2].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[2].texture.sampleType = WGPUTextureSampleType::WGPUTextureSampleType_Float;
    bindingLayoutEntries[2].texture.viewDimension = WGPUTextureViewDimension::WGPUTextureViewDimension_2D;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

std::vector<WGPUBindGroup> Isometric::OnBindGroups() {
    std::vector<WGPUBindGroup> bindGroups(1);

    std::vector<WGPUBindGroupEntry> bindGroupEntries(2);
    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].buffer = m_uniformBuffer.getBuffer();
    bindGroupEntries[0].offset = 0u;
    bindGroupEntries[0].size = sizeof(Uniforms);

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].buffer = m_skinBuffer.getBuffer();
    bindGroupEntries[1].offset = 0u;
    bindGroupEntries[1].size = wgpuBufferGetSize(m_skinBuffer.getBuffer());

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_ANIMATION"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    bindGroups[0] = wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);

    return bindGroups;
}

std::vector<WGPUBindGroup> Isometric::OnBindGroupsTexture() {
    std::vector<WGPUBindGroup> bindGroups(1);

    std::vector<WGPUBindGroupEntry> bindGroupEntries(3);
    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].buffer = m_uniformBuffer.getBuffer();
    bindGroupEntries[0].offset = 0u;
    bindGroupEntries[0].size = sizeof(Uniforms);

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].sampler = wgpContext.getSampler(SS_LINEAR_REPEAT);

    bindGroupEntries[2].binding = 2u;
    bindGroupEntries[2].textureView = m_wgpFloorD.getTextureView();

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_TEXTURE"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    bindGroups[0] = wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);

    return bindGroups;
}