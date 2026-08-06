#include <WebGPU/WgpContext.h>
#include <WebGPU/WgpRenderer.h>

#include <Nuklear/NkContext.h>
#include <Nuklear/NkStyle.h>

#include <States/VolumeRendering.h>
#include <States/Collada.h>

#include "InputTouch.h"
#include "Isometric.h"
#include "Logging.h"
#include "DeltaClock.h"

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

ThreadPool threadPool(4);

Isometric::Isometric(StateMachine& machine) : State(machine, States::ISOMETRIC), m_bulletStore(&threadPool) {

    nkInit(static_cast<float>(wgpWidth), static_cast<float>(wgpHeight));
    nkInitFont("fonts/upheavtt.ttf");

    m_camera.perspective(glm::radians(45.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 100.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), 0.0f, static_cast<float>(wgpHeight), -1.0f, 1.0f);
    m_camera.lookAt(glm::vec3(0.0f, 4.3f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_camera.setMovingSpeed(50.0f);
    m_camera.setRotationSpeed(0.1f);

    m_trackball.reshape(wgpWidth, wgpHeight);
    m_floor.buildQuadXZ({-50.0f, 0.0f, -50.0f}, {100.0f, 100.0f});
    m_bullet.buildQuadXZ({ -0.3f * 0.243f, 0.0f, -0.3f * 0.243f }, { 0.3f * 0.5f, 0.3f * 0.5f }, 1u, 1u, true, false);

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
    m_player.scale(0.0044f, 0.0044f, 0.0044f);
    m_rotationButtonResult.degrees = 90.0f;

    m_enemy.loadModel("models/EelDog/EelDog.fbx");
    m_enemy.rotate(90.0f, 0.0f, 0.0f);
    m_enemy.rotate(0.0f, 180.0f, 0.0f);
    m_enemy.scale(0.01f);

    Material::CleanupMaterials();
    static_cast<const AssimpMesh*>(m_enemy.getMesh())->setMaterialIndex(-1);

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

    m_player.addAnimationState(AnimationManager::Get().getAnimation("forward"));
    m_player.getAnimationState(0u)->setLooped(true);
    m_player.addAnimationState(AnimationManager::Get().getAnimation("left"));
    m_player.getAnimationState(1u)->setLooped(true);
    m_player.addAnimationState(AnimationManager::Get().getAnimation("backward"));
    m_player.getAnimationState(2u)->setLooped(true);
    m_player.addAnimationState(AnimationManager::Get().getAnimation("right"));
    m_player.getAnimationState(3u)->setLooped(true);
    m_player.addAnimationState(AnimationManager::Get().getAnimation("idle"));
    m_player.getAnimationState(4u)->setLooped(true);
    m_player.addAnimationState(AnimationManager::Get().getAnimation("death"));
    m_player.getAnimationState(5u)->setLooped(false);
    m_player.update(0.1f);

    m_uniformBuffer.createBuffer(sizeof(Uniforms), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);
    m_instanceBuffer.createBuffer(sizeof(Uniforms), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);
    m_wigglyBuffer.createBuffer(sizeof(glm::vec4), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);
    m_skinBuffer.createBuffer(sizeof(glm::mat4) * 96u, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage);
    m_rotationBuffer.createBuffer(sizeof(glm::vec4) * 4000u, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);
    m_offsetBuffer.createBuffer(sizeof(glm::vec4) * 4000u, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);

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

    m_wgpBulletTexture.setFlipHorizontal(true);
    m_wgpBulletTexture.loadFromFile("textures/BulletTexture.png");
    m_wgpFloorD.loadFromFile("textures/floor/Floor_D.psd");
    m_wgpEnemyD.loadFromFile("models/EelDog/Eeldog_Albedo.tif");

    wgpContext.addSahderModule("ANIMATION", "shader/animation_fbx.wgsl");
    wgpContext.createRenderPipeline("ANIMATION", "RP_ANIMATION", VL_PTNWJ, std::bind(&Isometric::OnBindGroupLayouts, this));

    wgpContext.addSahderModule("FLOOR", "shader/floor.wgsl");
    wgpContext.createRenderPipeline("FLOOR", "RP_FLOOR", VL_PTN, std::bind(&Isometric::OnBindGroupLayoutsFloor, this));

    wgpContext.addSahderModule("WIGGLY", "shader/wiggly.wgsl");
    wgpContext.createRenderPipeline("WIGGLY", "RP_WIGGLY", VL_PTN, std::bind(&Isometric::OnBindGroupLayoutsWiggly, this));

    wgpContext.addSahderModule("BULLET", "shader/bullet.wgsl");
    wgpContext.createRenderPipeline("BULLET", "RP_BULLET", VL_PT, std::bind(&Isometric::OnBindGroupLayoutsBullet, this),
                                    1u, WGPUPrimitiveTopology_TriangleList, WGPUTextureFormat_Undefined, WGPUTextureFormat_Undefined, WGPUCompareFunction_Always,
                                    { DEPTH_STENCIL_STATE | BLEND_STATE | FRAGMENT_STATE, BlendMode::ALPHA_BLENDING, WGPUTextureFormat_Undefined, WGPUCullMode_None, StencilMode::DEFAULT, {} });

    m_wgpPlayer.create(m_player);
    m_wgpPlayer.setBindGroups("BG", std::bind(&Isometric::OnBindGroups, this));

    m_wgpFloor.create(m_floor);
    m_wgpFloor.setBindGroups("BG", std::bind(&Isometric::OnBindGroupsFloor, this));

    m_wgpBullet.create(m_bullet);
    m_wgpBullet.setBindGroups("BG", std::bind(&Isometric::OnBindGroupsBullet, this));

    m_wgpEnemy.create(m_enemy);
    m_wgpEnemy.addBindGroup("BG", CreateBindGroup(m_instanceBuffer, m_wigglyBuffer, m_wgpEnemyD));

    wgpContext.setClearColor({ 0.2f, 0.2f, 0.2f, 1.0f });
    wgpContext.OnDraw = std::bind(&Isometric::OnDraw, this, std::placeholders::_1, std::placeholders::_2);
    nkContext.OnFillBuffer = std::bind(&Isometric::OnFillBuffer, this, std::placeholders::_1);
}

Isometric::~Isometric() {
    m_uniformBuffer.markForDelete();
    m_skinBuffer.markForDelete();
}

void Isometric::fixedUpdate() {

}

void Isometric::update() {
	nkUpdateInput(0, 0, false, false, 0.0f);

    if (m_rotationButtonResult.buttonDown && (lastFireTime + 0.1f) < Clock()) {
        const glm::quat midOri = m_player.getOrientation();
        const glm::mat4 playerModelTransform = m_player.getWorldTransformation();
        const glm::vec3 projectileSpawnPoint = playerModelTransform * glm::vec4(-20.0f, 120.0f, 100.0f, 1.0f);

        m_bulletStore.createBullets(projectileSpawnPoint, midOri, 10);
        lastFireTime = Clock();
    }

    m_trackball.idle();

    const glm::vec3 posistion = static_cast<const AnimatedMesh*>(m_player.getMesh())->getBone(0u).getPosition();
    m_camera.lookAt(posistion + glm::vec3(0.0f, 4.3f, 4.0f), posistion, glm::vec3(0.0f, 1.0f, 0.0f));

    if(m_rotationButtonResult.degrees != 0.0f && !m_isDeath)
        m_player.setOrientation(0.0f, m_rotationButtonResult.degrees, 0.0f);

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

    glm::vec3 playerDirection = glm::vec3(0.0f, 0.0f, 0.0f);
    if (moveY > 0.0f && !m_isDeath) {
        playerDirection -= glm::vec3(0.0f, 0.0f, 1.0f);
    }

    if (moveY < 0.0f && !m_isDeath) {
        playerDirection += glm::vec3(0.0f, 0.0f, 1.0f);
    }

    if (moveX < 0.0f && !m_isDeath) {
        playerDirection -= glm::vec3(1.0f, 0.0f, 0.0f);
    }

    if (moveX > 0.0f && !m_isDeath) {
        playerDirection += glm::vec3(1.0f, 0.0f, 0.0f);
    }

    playerMove = glm::length2(playerDirection) > 0.01f && !m_isDeath;

    if (playerMove) {
        m_player.translate(playerDirection[0] * 2.0f * m_dt, playerDirection[1] * 2.0f * m_dt, playerDirection[2] * 2.0f * m_dt);
    }

    float movementTheta = std::atan2(-playerDirection[2], playerDirection[0]);

    if (movementTheta < 0.0f)
        movementTheta += glm::pi<float>() * 2.0f;

    const float thetaDelta = movementTheta - glm::radians(m_rotationButtonResult.degrees);
    const glm::vec2 movementAnim = !playerMove ? glm::vec2(0.0f, 0.0f) : glm::vec2(cosf(thetaDelta), -sinf(thetaDelta));

    prev_idleWeight = std::max(0.0f, prev_idleWeight - m_dt / animTransitionTime);
    prev_rightWeight = std::max(0.0f, prev_rightWeight - m_dt / animTransitionTime);
    prev_leftWeight = std::max(0.0f, prev_leftWeight - m_dt / animTransitionTime);
    prev_forwardWeight = std::max(0.0f, prev_forwardWeight - m_dt / animTransitionTime);
    prev_backWeight = std::max(0.0f, prev_backWeight - m_dt / animTransitionTime);

    float deathWeight = m_isDeath ? 1.0f : 0.0f;
    float idleWeight = prev_idleWeight + ((m_isDeath || playerMove) ? 0.0f : 1.0f);
    float forwardWeight = prev_forwardWeight + (playerMove ? std::max(0.0f, movementAnim[1]) : 0.0f);
    float leftWeight = prev_leftWeight + (playerMove ? std::max(0.0f, movementAnim[0]) : 0.0f);
    float backWeight = prev_backWeight + (playerMove ? std::max(0.0f, -movementAnim[1]) : 0.0f);
    float rightWeight = prev_rightWeight + (playerMove ? std::max(0.0f, -movementAnim[0]) : 0.0f);
    const float weightSum = deathWeight + idleWeight + rightWeight + forwardWeight + backWeight + leftWeight;

    deathWeight /= weightSum;
    idleWeight /= weightSum;
    rightWeight /= weightSum;
    forwardWeight /= weightSum;
    backWeight /= weightSum;
    leftWeight /= weightSum;
    prev_idleWeight = std::max(prev_idleWeight, idleWeight);
    prev_rightWeight = std::max(prev_rightWeight, rightWeight);
    prev_leftWeight = std::max(prev_leftWeight, leftWeight);
    prev_forwardWeight = std::max(prev_forwardWeight, forwardWeight);
    prev_backWeight = std::max(prev_backWeight, backWeight);

    idleWeight *= 0.25f;

    m_player.getAnimationState(0u)->setWeight(forwardWeight);
    m_player.getAnimationState(1u)->setWeight(leftWeight);
    m_player.getAnimationState(2u)->setWeight(backWeight);
    m_player.getAnimationState(3u)->setWeight(rightWeight);
    m_player.getAnimationState(4u)->setWeight(idleWeight);
    m_player.getAnimationState(5u)->setWeight(deathWeight);

    m_player.update(m_dt);
    m_player.updateSkinning();
    m_bulletStore.updateBullets(m_dt);

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

    glm::vec3 posEnemy = glm::vec3(2.0f, 120.0f * 0.0044f, 2.0f);
    m_uniforms.model = glm::translate(posEnemy) * glm::eulerAngleXYZ(0.0f, glm::radians(getLookAtYRotation(posistion, posEnemy)), 0.0f);
    wgpuQueueWriteBuffer(wgpContext.queue, m_instanceBuffer.getBuffer(), 0u, &m_uniforms, sizeof(Uniforms));

    m_wiggly.nosePos[0] = 1.0f ;
    m_wiggly.nosePos[1] = 120.0f * 0.0044f ;
    m_wiggly.nosePos[2] = -2.0f ;
    m_wiggly.time = Clock();
    wgpuQueueWriteBuffer(wgpContext.queue, m_wigglyBuffer.getBuffer(), 0, &m_wiggly, sizeof(Wiggly));

    wgpuQueueWriteBuffer(wgpContext.queue, m_rotationBuffer.getBuffer(), 0u, m_bulletStore.m_rots.data(), m_bulletStore.m_rots.size() * sizeof(glm::vec4));
    wgpuQueueWriteBuffer(wgpContext.queue, m_offsetBuffer.getBuffer(), 0u, m_bulletStore.m_offsets.data(), m_bulletStore.m_offsets.size() * sizeof(glm::vec4));
}

void Isometric::render() {
    wgpDraw();
}

void Isometric::OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor) {
    {
        WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);
        wgpuRenderPassEncoderSetViewport(renderPassEncoder, 0.0f, 0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f, 1.0f);

        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_FLOOR"));
        m_wgpFloor.draw(renderPassEncoder);

        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_WIGGLY"));
        m_wgpEnemy.draw(renderPassEncoder);

        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_ANIMATION"));
        m_wgpPlayer.draw(renderPassEncoder);

        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_BULLET"));
        m_wgpBullet.draw(renderPassEncoder, m_bulletStore.m_rots.size());

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

std::vector<WGPUBindGroupLayout> Isometric::OnBindGroupLayoutsFloor() {
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

std::vector<WGPUBindGroupLayout> Isometric::OnBindGroupLayoutsWiggly() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(4);
    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[0].buffer.minBindingSize = sizeof(Uniforms);

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[1].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[1].buffer.minBindingSize = sizeof(glm::vec4);

    bindingLayoutEntries[2].binding = 2u;
    bindingLayoutEntries[2].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[2].sampler.type = WGPUSamplerBindingType::WGPUSamplerBindingType_Filtering;

    bindingLayoutEntries[3].binding = 3u;
    bindingLayoutEntries[3].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[3].texture.sampleType = WGPUTextureSampleType::WGPUTextureSampleType_Float;
    bindingLayoutEntries[3].texture.viewDimension = WGPUTextureViewDimension::WGPUTextureViewDimension_2D;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

std::vector<WGPUBindGroupLayout> Isometric::OnBindGroupLayoutsBullet() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(5);

    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[0].buffer.minBindingSize = sizeof(Uniforms);

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[1].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[1].buffer.minBindingSize = sizeof(glm::vec4) * 4000u;

    bindingLayoutEntries[2].binding = 2u;
    bindingLayoutEntries[2].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[2].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[2].buffer.minBindingSize = sizeof(glm::vec4) * 4000u;

    bindingLayoutEntries[3].binding = 3u;
    bindingLayoutEntries[3].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[3].sampler.type = WGPUSamplerBindingType::WGPUSamplerBindingType_Filtering;

    bindingLayoutEntries[4].binding = 4u;
    bindingLayoutEntries[4].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[4].texture.sampleType = WGPUTextureSampleType::WGPUTextureSampleType_Float;
    bindingLayoutEntries[4].texture.viewDimension = WGPUTextureViewDimension::WGPUTextureViewDimension_2D;

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

std::vector<WGPUBindGroup> Isometric::OnBindGroupsFloor() {
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
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_FLOOR"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    bindGroups[0] = wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);

    return bindGroups;
}

std::vector<WGPUBindGroup> Isometric::OnBindGroupsBullet() {
    std::vector<WGPUBindGroup> bindGroups(1);

    std::vector<WGPUBindGroupEntry> bindGroupEntries(5);

    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].buffer = m_uniformBuffer.getBuffer();
    bindGroupEntries[0].offset = 0u;
    bindGroupEntries[0].size = wgpuBufferGetSize(m_uniformBuffer.getBuffer());

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].buffer = m_rotationBuffer.getBuffer();
    bindGroupEntries[1].offset = 0u;
    bindGroupEntries[1].size = wgpuBufferGetSize(m_rotationBuffer.getBuffer());

    bindGroupEntries[2].binding = 2u;
    bindGroupEntries[2].buffer = m_offsetBuffer.getBuffer();
    bindGroupEntries[2].offset = 0u;
    bindGroupEntries[2].size = wgpuBufferGetSize(m_rotationBuffer.getBuffer());

    bindGroupEntries[3].binding = 3u;
    bindGroupEntries[3].sampler = wgpContext.getSampler(SS_LINEAR_CLAMP);

    bindGroupEntries[4].binding = 4u;
    bindGroupEntries[4].textureView = m_wgpBulletTexture.getTextureView();

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_BULLET"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    bindGroups[0] = wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);

    return bindGroups;
}

WGPUBindGroup Isometric::CreateBindGroup(const WgpBuffer& uniformBuffer, const WgpBuffer& wigglyBuffer, const WgpTexture& texture) {
    std::vector<WGPUBindGroupEntry> bindGroupEntries(4);
    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].buffer = uniformBuffer.getBuffer();
    bindGroupEntries[0].offset = 0u;
    bindGroupEntries[0].size = sizeof(Uniforms);

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].buffer = wigglyBuffer.getBuffer();
    bindGroupEntries[1].offset = 0u;
    bindGroupEntries[1].size = sizeof(glm::vec4);

    bindGroupEntries[2].binding = 2u;
    bindGroupEntries[2].sampler = wgpContext.getSampler(SS_LINEAR_REPEAT);

    bindGroupEntries[3].binding = 3u;
    bindGroupEntries[3].textureView = texture.getTextureView();

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_WIGGLY"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    return wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
}

float Isometric::getLookAtYRotation(const glm::vec3& objectPos, const glm::vec3& targetPos) {
    float dx = targetPos[0] - objectPos[0];
    float dz = targetPos[2] - objectPos[2];

    if (abs(dx) < 0.01f && abs(dz) < 0.01f)
        return 0.0f;

    return glm::degrees(std::atan2(dx, dz));
}