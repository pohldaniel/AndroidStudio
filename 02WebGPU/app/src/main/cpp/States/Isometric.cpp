#include <glm/gtc/type_ptr.hpp>

#include <WebGPU/WgpContext.h>
#include <WebGPU/WgpRenderer.h>

#include <Nuklear/NkContext.h>
#include <Nuklear/NkStyle.h>

#include <States/Cubes.h>
#include <States/Collada.h>

#include <core/scene/CollisionNode.h>

#include <core/entities/CollisionEntity.h>
#include <core/entities/Enemy.h>
#include <core/entities/Player.h>

#include "InputTouch.h"
#include "Isometric.h"
#include "Logging.h"
#include "Globals.h"
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

Isometric::Isometric(StateMachine& machine) : State(machine, States::ISOMETRIC), m_bulletStore(&threadPool), m_enemySpawner(120.0f * 0.0044f, m_player) {

    nkInit(static_cast<float>(wgpWidth), static_cast<float>(wgpHeight));
    nkInitFont("fonts/upheavtt.ttf");
    Physics::DebugDrawer.init();

    m_camera.perspective(glm::radians(45.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 100.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), 0.0f, static_cast<float>(wgpHeight), -1.0f, 1.0f);
    m_camera.lookAt(glm::vec3(0.0f, 4.3f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_camera.setMovingSpeed(50.0f);
    m_camera.setRotationSpeed(0.1f);

    m_trackball.reshape(wgpWidth, wgpHeight);
    m_floor.buildQuadXZ({-50.0f, 0.0f, -50.0f}, {100.0f, 100.0f});
    m_floor.rotate(0.0f, 45.0f, 0.0f);
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

    m_fire.init<OboeEffect>();
    m_ding.init<OboeEffect>();
    m_ding.get<OboeEffect>()->getMixer().setVolume(0.5f);

    m_uniformBuffer.createBuffer(sizeof(Uniforms), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);
    m_infoBufferBillboard.createBuffer(sizeof(FrameInfo), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);
    m_infoBufferMuzzle.createBuffer(sizeof(FrameInfo), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);

    m_storageBuffer.createBuffer(1000u * sizeof(glm::mat4), WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    m_wigglyBuffer.createBuffer(sizeof(glm::vec4), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);
    m_skinBuffer.createBuffer(sizeof(glm::mat4) * 96u, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage);
    m_rotationBuffer.createBuffer(sizeof(glm::vec4) * 4000u, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);
    m_offsetBuffer.createBuffer(sizeof(glm::vec4) * 4000u, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);

    m_spriteBuffer.createBuffer(20u * sizeof(SpriteInstance), WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    m_muzzleBuffer.createBuffer(100u * sizeof(SpriteInstance), WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);

    m_lightDir = glm::vec3(-1.0f, -1.0f, -1.0f);
    m_lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 50.0f);
    m_lightView = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f) - 20.0f * m_lightDir, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    m_uniforms.projection = m_camera.getPerspectiveMatrix();
    m_uniforms.view = m_camera.getViewMatrix();
    m_uniforms.env = m_camera.getRotationMatrix();
    m_uniforms.model = glm::mat4(1.0f);
    m_uniforms.normal = Camera::GetNormalMatrix(m_camera.getViewMatrix() * m_uniforms.model);
    m_uniforms.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    m_uniforms.camPosition = m_camera.getPosition();
    m_uniforms.lightVP = m_lightProjection * m_lightView;
    m_uniforms.shadow = Camera::BIAS *  m_uniforms.lightVP;
    m_uniforms.lightPosition = glm::vec3(0.0f, 0.0f, 0.0f) - 20.0f * m_lightDir;

    wgpuQueueWriteBuffer(wgpContext.queue, m_uniformBuffer.getBuffer(), 0, &m_uniforms, sizeof(Uniforms));

    FrameInfo sprite;
    sprite.frameSize[0] = 1.0f / 11.0f;
    sprite.frameSize[1] = 1.0f;
    sprite.colRow[0] = 11u;
    sprite.colRow[1] = 1u;

    wgpuQueueWriteBuffer(wgpContext.queue, m_infoBufferBillboard.getBuffer(), 0u, &sprite, sizeof(FrameInfo));

    FrameInfo muzzle;
    muzzle.frameSize[0] = 1.0f / 6.0f;
    muzzle.frameSize[1] = 1.0f;
    muzzle.colRow[0] = 6u;
    muzzle.colRow[1] = 1u;

    wgpuQueueWriteBuffer(wgpContext.queue, m_infoBufferMuzzle.getBuffer(), 0u, &muzzle, sizeof(FrameInfo));

    m_wgpBulletTexture.setFlipHorizontal(true);
    m_wgpBulletTexture.loadFromFile("textures/BulletTexture.png");
    m_wgpFloorD.loadFromFile("textures/floor/Floor_D.psd");
    m_wgpEnemyD.loadFromFile("models/EelDog/Eeldog_Albedo.tif");
    m_sprite.loadFromFile("textures/impact_spritesheet_with_00.png");
    m_muzzle.loadFromFile("textures/muzzle_spritesheet.png");
    m_wgpTextureShadow.createEmpty(4u * 1024u, 4u * 1024u, 1u, WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment, WGPUTextureFormat_Depth32Float);

    wgpContext.addSampler(wgpCreateSampler(WGPUFilterMode_Linear, WGPUAddressMode_ClampToEdge, 1u, WGPUMipmapFilterMode_Nearest, WGPUCompareFunction_Greater), SS_0);
    wgpContext.addSahderModule("ANIMATION", "shader/player.wgsl");
    wgpContext.createRenderPipeline("ANIMATION", "RP_ANIMATION", VL_PTNWJ, std::bind(&Isometric::OnBindGroupLayouts, this));

    wgpContext.addSahderModule("FLOOR", "shader/floor.wgsl");
    wgpContext.createRenderPipeline("FLOOR", "RP_FLOOR", VL_PTN, std::bind(&Isometric::OnBindGroupLayoutsFloor, this));

    wgpContext.addSahderModule("WIGGLY", "shader/wiggly.wgsl");
    wgpContext.createRenderPipeline("WIGGLY", "RP_WIGGLY", VL_PTN, std::bind(&Isometric::OnBindGroupLayoutsWiggly, this));

    wgpContext.addSahderModule("BULLET", "shader/bullet.wgsl");
    wgpContext.createRenderPipeline("BULLET", "RP_BULLET", VL_PT, std::bind(&Isometric::OnBindGroupLayoutsBullet, this),
                                    1u, WGPUPrimitiveTopology_TriangleList, WGPUTextureFormat_Undefined, WGPUTextureFormat_Undefined, WGPUCompareFunction_Always,
                                    { DEPTH_STENCIL_STATE | BLEND_STATE | FRAGMENT_STATE, BlendMode::ALPHA_BLENDING, WGPUTextureFormat_Undefined, WGPUCullMode_None, StencilMode::DEFAULT, {} });

    wgpContext.addSahderModule("BILLBOARD", "shader/billboard.wgsl");
    wgpContext.createRenderPipeline("BILLBOARD", "RP_BILLBOARD", VL_NONE, std::bind(&Isometric::OnBindGroupLayoutsBillboard, this),
                                    1u, WGPUPrimitiveTopology_TriangleStrip, WGPUTextureFormat_Undefined, WGPUTextureFormat_Undefined, WGPUCompareFunction_Always,
                                    { DEPTH_STENCIL_STATE | BLEND_STATE | FRAGMENT_STATE, BlendMode::ALPHA_BLENDING, WGPUTextureFormat_Undefined, WGPUCullMode_None, StencilMode::DEFAULT, {} });

    wgpContext.addSahderModule("MUZZLE", "shader/muzzle.wgsl");
    wgpContext.createRenderPipeline("MUZZLE", "RP_MUZZLE", VL_NONE, std::bind(&Isometric::OnBindGroupLayoutsBillboard, this),
                                    1u, WGPUPrimitiveTopology_TriangleStrip, WGPUTextureFormat_Undefined, WGPUTextureFormat_Undefined, WGPUCompareFunction_Always,
                                    { DEPTH_STENCIL_STATE | BLEND_STATE | FRAGMENT_STATE, BlendMode::ALPHA_BLENDING, WGPUTextureFormat_Undefined, WGPUCullMode_None, StencilMode::DEFAULT, {} });

    wgpContext.addSahderModule("PLAYER_SHADOW", "shader/player_shadow.wgsl");
    wgpContext.createRenderPipeline("PLAYER_SHADOW", "RP_PLAYER_SHADOW", VL_PTNWJ, std::bind(&Isometric::OnBindGroupLayoutsShadow, this),
                                    1u, WGPUPrimitiveTopology_TriangleList, WGPUTextureFormat_Undefined, WGPUTextureFormat_Depth32Float, WGPUCompareFunction_Less,
                                    { WRITE_DEPTH | DEPTH_STENCIL_STATE, BlendMode::ALPHA_BLENDING }
    );

    wgpContext.addSahderModule("WIGGLY_SHADOW", "shader/wiggly_shadow.wgsl");
    wgpContext.createRenderPipeline("WIGGLY_SHADOW", "RP_WIGGLY_SHADOW", VL_PTN, std::bind(&Isometric::OnBindGroupLayoutsWigglyShadow, this),
                                    1u, WGPUPrimitiveTopology_TriangleList, WGPUTextureFormat_Undefined, WGPUTextureFormat_Depth32Float, WGPUCompareFunction_Less,
                                    { WRITE_DEPTH | DEPTH_STENCIL_STATE, BlendMode::ALPHA_BLENDING }
    );

    m_wgpPlayer.create(m_player);
    m_wgpPlayer.setBindGroups("SHADOW", std::bind(&Isometric::OnBindGroupsShadow, this));
    m_wgpPlayer.setBindGroups("BG", std::bind(&Isometric::OnBindGroups, this));

    m_wgpFloor.create(m_floor);
    m_wgpFloor.setBindGroups("BG", std::bind(&Isometric::OnBindGroupsFloor, this));

    m_wgpBullet.create(m_bullet);
    m_wgpBullet.setBindGroups("BG", std::bind(&Isometric::OnBindGroupsBullet, this));

    m_wgpEnemy.create(m_enemy);
    m_wgpEnemy.addBindGroup("SHADOW", CreateBindGroupShadow(m_uniformBuffer, m_wigglyBuffer, m_storageBuffer));
    m_wgpEnemy.addBindGroup("BG", createBindGroupWiggly());

    wgpContext.setClearColor({ 0.2f, 0.2f, 0.2f, 1.0f });
    wgpContext.OnDraw = std::bind(&Isometric::OnDraw, this, std::placeholders::_1, std::placeholders::_2);
    nkContext.OnFillBuffer = std::bind(&Isometric::OnFillBuffer, this, std::placeholders::_1);

    m_scene = new SceneNode();
    m_scene->setOnChildAdded([this](Node* newNode) {
        if (auto* enemy = dynamic_cast<Enemy*>(newNode)) {
            m_enemies.push_back(enemy);
            m_enemySpawner.count()++;
        }
    });

    m_scene->setOnChildRemoved([this](Node* removedNode) {
        auto it = std::find(m_enemies.begin(), m_enemies.end(), removedNode);
        if (it != m_enemies.end()) {
            std::iter_swap(it, m_enemies.end() - 1);
            m_enemies.pop_back();
            m_enemySpawner.count()--;
        }
    });

    m_enemySpawner.scene = m_scene;
    m_targetPoolSize = 100;

    btCollisionObject* body = Physics::AddKinematicObject(Physics::BtTransform(glm::vec3(0.0f, 0.4f, 0.0f)), new btCylinderShape(btVector3(0.35f * 0.5f, 0.4f, 0.35f * 0.5f)), Physics::collisiontypes::CHARACTER, Physics::collisiontypes::ENEMY);
    m_playerEnitity = m_scene->addChild<Player>(body, m_player);

    m_bindGroupBillboard = createBindGroupBillboard();
    m_bindGroupMuzzle = createBindGroupMuzzle();
    m_muzzleInstance.currentFrame = 6u;
}

Isometric::~Isometric() {
    delete m_scene;
    m_uniformBuffer.markForDelete();
    m_infoBufferBillboard.markForDelete();
    m_infoBufferMuzzle.markForDelete();
    m_storageBuffer.markForDelete();
    m_wigglyBuffer.markForDelete();
    m_skinBuffer.markForDelete();
    m_rotationBuffer.markForDelete();
    m_offsetBuffer.markForDelete();
    m_spriteBuffer.markForDelete();
    m_muzzleBuffer.markForDelete();

    m_wgpFloorD.markForDelete();
    m_wgpEnemyD.markForDelete();
    m_wgpBulletTexture.markForDelete();
    m_sprite.markForDelete();
    m_muzzle.markForDelete();
    m_wgpTextureShadow.markForDelete();

    wgpuBindGroupRelease(m_bindGroupBillboard);
    wgpuBindGroupRelease(m_bindGroupMuzzle);
}

void Isometric::fixedUpdate() {
    size_t activeBulletCount = m_bulletStore.m_offsets.size();

    while (m_entities.size() < activeBulletCount) {
        createNewBulletToPool();
    }

    while (m_entities.size() > activeBulletCount) {
        CollisionEntity* lastEntity = m_entities.back();
        m_scene->eraseChildSilent(lastEntity);
        m_entities.pop_back();
    }

    for (auto entity : m_entities) {
        entity->setActive(false);
    }

    for (size_t i = 0; i < activeBulletCount; ++i) {
        glm::vec3 pos = m_bulletStore.m_offsets[i];
        glm::quat rot = m_bulletStore.m_rots[i];

        m_entities[i]->setActive(true);
        m_entities[i]->setPosition(pos[0], pos[1], pos[2]);
        m_entities[i]->setOrientation(rot.x, rot.y, rot.z, rot.w);
    }

    for (size_t i = activeBulletCount; i < m_entities.size(); ++i) {
        if (m_entities[i]->isActive()) {
            m_entities[i]->setActive(false);
        }
    }

    for (auto entity : m_entities) {
        entity->fixedUpdate(m_fdt);
    }

    for (auto enemy : m_enemies) {
        enemy->fixedUpdate(m_fdt);
    }

    m_playerEnitity->fixedUpdate(m_fdt);

    Globals::physics->stepSimulation(FIXED_STEP);

    BulletCollisionPlayerCallback callback;
    Physics::GetDynamicsWorld()->contactTest(m_playerEnitity->getCollisionObject(), callback);
    if (callback.m_hasCollided && callback.m_hitTarget) {
        m_playerEnitity->setActive(false);
        m_isDeath = true;
    }

    for (auto entity : m_entities) {
        if (!entity->isActive()) continue;

        BulletCollisionCallback callback;
        Physics::GetDynamicsWorld()->contactTest(entity->getCollisionObject(), callback);

        if (callback.m_hasCollided && callback.m_hitTarget) {
            void* userPtr = callback.m_hitTarget->getUserPointer();
            if (userPtr) {
                Enemy* hitEnemy = static_cast<Enemy*>(userPtr);
                hitEnemy->setActive(false);
                m_scene->eraseChild(hitEnemy);

                m_ding.play("sounds/bullet_hit_metal_enemy_4.wav");
                spawnBillboard(hitEnemy->getPosition());
            }
        }
    }
}

void Isometric::update() {
	nkUpdateInput(0, 0, false, false, 0.0f);

    if (!m_isDeath && m_rotationButtonResult.buttonDown && (lastFireTime + 0.1f) < Clock()) {
        const glm::quat midOri = m_player.getOrientation();
        const glm::mat4 playerModelTransform = m_player.getWorldTransformation();
        const glm::vec3 projectileSpawnPoint = playerModelTransform * glm::vec4(-20.0f, 120.0f, 100.0f, 1.0f);

        m_bulletStore.createBullets(projectileSpawnPoint, midOri, 10);
        lastFireTime = Clock();
        m_fire.play("sounds/shooting_one.wav");
        resetMuzzle();
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
        m_playerEnitity->translate(playerDirection[0] * 2.0f * m_dt, playerDirection[1] * 2.0f * m_dt, playerDirection[2] * 2.0f * m_dt);
        m_lightView = glm::lookAt(m_playerEnitity->getPosition() - 20.0f * m_lightDir, m_playerEnitity->getPosition(), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    float movementTheta = std::atan2(playerDirection[0], playerDirection[2]);
    const float thetaDelta = movementTheta - glm::radians(m_rotationButtonResult.degrees);
    const glm::vec2 movementAnim = !playerMove ? glm::vec2(0.0f, 0.0f) : glm::vec2(sinf(thetaDelta), cos(thetaDelta));

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
    m_enemySpawner.update(posistion, m_dt);
    for (auto enemy : m_enemies) {
        enemy->update(m_dt);
    }
    updateBillboards(m_dt);
    updateMuzzle(m_dt);

    const AnimatedMesh* mesh = static_cast<const AnimatedMesh*>(m_player.getMesh());
    mesh->skinMatrices()[42] = mesh->getBone(43u).getWorldTransformation() * offset * pivot * mesh->skinMatrices()[42];
    wgpuQueueWriteBuffer(wgpContext.queue, m_skinBuffer.getBuffer(), 0u, mesh->getSkinMatrices(), mesh->getNumBones() * sizeof(glm::mat4));

    glm::mat4 muzzleTransform = mesh->skinMatrices()[42] * glm::translate(glm::vec3(214.0f, 76.143f, -3.054f)) * glm::scale(glm::vec3(50.0f, 50.0f, 50.0f));
    float angle = aimTheta;
    while (angle < 0.0f) angle += 360.0f;
    while (angle >= 360.0f) angle -= 360.0f;
    float radians = glm::radians(angle);
    float finalCorrectionAngle = 90.0f * std::abs(std::cos(radians));
    float sign = (angle > 0.0f && angle < 180.0f) ? -1.0f : 1.0f;
    muzzleTransform = muzzleTransform * glm::rotate(glm::radians(sign * finalCorrectionAngle), glm::vec3(1.0f, 0.0f, 0.0f));

    m_uniforms.projection = m_camera.getPerspectiveMatrix();
    m_uniforms.view = m_camera.getViewMatrix();
    m_uniforms.env = m_camera.getRotationMatrix();
    m_uniforms.model = muzzleTransform;
    m_uniforms.normal = Camera::GetNormalMatrix(m_camera.getViewMatrix() * m_uniforms.model);
    m_uniforms.camPosition = m_camera.getPosition();
    m_uniforms.lightVP = m_lightProjection * m_lightView;
    m_uniforms.shadow = Camera::BIAS * m_uniforms.lightVP;
    m_uniforms.lightPosition = m_playerEnitity->getPosition() - 20.0f * m_lightDir;
    wgpuQueueWriteBuffer(wgpContext.queue, m_uniformBuffer.getBuffer(), 0, &m_uniforms, sizeof(Uniforms));

    m_cpuInstanceBuffer.clear();
    m_cpuInstanceBuffer.reserve(m_enemies.size());

    for (const auto& child : m_enemies) {
        m_cpuInstanceBuffer.push_back(child->getTransformationSOP());
    }

    wgpuQueueWriteBuffer(wgpContext.queue, m_storageBuffer.getBuffer(), 0u, m_cpuInstanceBuffer.data(), m_cpuInstanceBuffer.size() * sizeof(glm::mat4));

    m_wiggly.nosePos[0] = 1.0f ;
    m_wiggly.nosePos[1] = 120.0f * 0.0044f ;
    m_wiggly.nosePos[2] = -2.0f ;
    m_wiggly.time = Clock();
    wgpuQueueWriteBuffer(wgpContext.queue, m_wigglyBuffer.getBuffer(), 0, &m_wiggly, sizeof(Wiggly));

    wgpuQueueWriteBuffer(wgpContext.queue, m_rotationBuffer.getBuffer(), 0u, m_bulletStore.m_rots.data(), m_bulletStore.m_rots.size() * sizeof(glm::vec4));
    wgpuQueueWriteBuffer(wgpContext.queue, m_offsetBuffer.getBuffer(), 0u, m_bulletStore.m_offsets.data(), m_bulletStore.m_offsets.size() * sizeof(glm::vec4));

    if (m_debugCollision) {
        glm::mat4 viewProjection = m_camera.getPerspectiveMatrix() * m_camera.getViewMatrix();
        float* targetArray = Physics::DebugDrawer.getViewProjection();
        std::copy(glm::value_ptr(viewProjection), glm::value_ptr(viewProjection) + 16, targetArray);
    }

    wgpuQueueWriteBuffer(wgpContext.queue, m_spriteBuffer.getBuffer(), 0u, m_activeBillboards.data(), m_activeBillboards.size() * sizeof(SpriteInstance));
    wgpuQueueWriteBuffer(wgpContext.queue, m_muzzleBuffer.getBuffer(), 0u, &m_muzzleInstance, sizeof(SpriteInstance));
}

void Isometric::render() {
    wgpDraw();
}

void Isometric::OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor) {

    {
        WgpRenderer::DrawDepth(m_wgpTextureShadow, std::bind(&Isometric::OnDrawShadow, this, std::placeholders::_1));
    }

    {
        WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);
        wgpuRenderPassEncoderSetViewport(renderPassEncoder, 0.0f, 0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f, 1.0f);

        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_FLOOR"));
        m_wgpFloor.draw(renderPassEncoder);

        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_WIGGLY"));
        m_wgpEnemy.draw(renderPassEncoder, m_cpuInstanceBuffer.size());

        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_ANIMATION"));
        m_wgpPlayer.draw(renderPassEncoder);

        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_BILLBOARD"));
        wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0u, m_bindGroupBillboard, 0u, NULL);
        wgpuRenderPassEncoderDraw(renderPassEncoder, 4u, m_activeBillboards.size(), 0u, 0u);

        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_MUZZLE"));
        wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0u, m_bindGroupMuzzle, 0u, NULL);
        wgpuRenderPassEncoderDraw(renderPassEncoder, 4u, 1u, 0u, 0u);

        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_BULLET"));
        m_wgpBullet.draw(renderPassEncoder, m_bulletStore.m_rots.size());

        wgpuRenderPassEncoderEnd(renderPassEncoder);
        wgpuRenderPassEncoderRelease(renderPassEncoder);
    }

    if (m_debugCollision)
    {
        Physics::GetDynamicsWorld()->debugDrawWorld();
        Physics::DebugDrawer.OnDraw(commandEncoder, renderPassDescriptor);
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
    Physics::DebugDrawer.shutDown();
    wgpCleanState();
    nkShutDown();
    m_isRunning = false;

    if(event.button == Event::MouseButtonEvent::BUTTON_LEFT){
        m_machine.addStateAtBottom(new Cubes(m_machine));
    }

    if(event.button == Event::MouseButtonEvent::BUTTON_RIGHT){
        m_machine.addStateAtBottom(new Collada(m_machine));
    }
}

std::vector<WGPUBindGroupLayout> Isometric::OnBindGroupLayouts() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(4);
    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[0].buffer.minBindingSize = sizeof(Uniforms);

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Vertex;
    bindingLayoutEntries[1].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_ReadOnlyStorage;
    bindingLayoutEntries[1].buffer.minBindingSize = 16 * sizeof(float);

    bindingLayoutEntries[2].binding = 2u;
    bindingLayoutEntries[2].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[2].sampler.type = WGPUSamplerBindingType::WGPUSamplerBindingType_Comparison;

    bindingLayoutEntries[3].binding = 3u;
    bindingLayoutEntries[3].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[3].texture.sampleType = WGPUTextureSampleType::WGPUTextureSampleType_Depth;
    bindingLayoutEntries[3].texture.viewDimension = WGPUTextureViewDimension::WGPUTextureViewDimension_2D;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

std::vector<WGPUBindGroupLayout> Isometric::OnBindGroupLayoutsFloor() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(5);
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

    bindingLayoutEntries[3].binding = 3u;
    bindingLayoutEntries[3].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[3].sampler.type = WGPUSamplerBindingType::WGPUSamplerBindingType_Comparison;

    bindingLayoutEntries[4].binding = 4u;
    bindingLayoutEntries[4].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[4].texture.sampleType = WGPUTextureSampleType::WGPUTextureSampleType_Depth;
    bindingLayoutEntries[4].texture.viewDimension = WGPUTextureViewDimension::WGPUTextureViewDimension_2D;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

std::vector<WGPUBindGroupLayout> Isometric::OnBindGroupLayoutsWiggly() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(7);
    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Vertex;
    bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[0].buffer.minBindingSize = sizeof(Uniforms);

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Vertex;
    bindingLayoutEntries[1].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[1].buffer.minBindingSize = sizeof(glm::vec4);

    bindingLayoutEntries[2].binding = 2u;
    bindingLayoutEntries[2].visibility = WGPUShaderStage_Vertex;
    bindingLayoutEntries[2].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_ReadOnlyStorage;
    bindingLayoutEntries[2].buffer.minBindingSize = 1000u * sizeof(glm::mat4);

    bindingLayoutEntries[3].binding = 3u;
    bindingLayoutEntries[3].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[3].sampler.type = WGPUSamplerBindingType::WGPUSamplerBindingType_Filtering;

    bindingLayoutEntries[4].binding = 4u;
    bindingLayoutEntries[4].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[4].texture.sampleType = WGPUTextureSampleType::WGPUTextureSampleType_Float;
    bindingLayoutEntries[4].texture.viewDimension = WGPUTextureViewDimension::WGPUTextureViewDimension_2D;

    bindingLayoutEntries[5].binding = 5u;
    bindingLayoutEntries[5].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[5].sampler.type = WGPUSamplerBindingType::WGPUSamplerBindingType_Comparison;

    bindingLayoutEntries[6].binding = 6u;
    bindingLayoutEntries[6].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[6].texture.sampleType = WGPUTextureSampleType::WGPUTextureSampleType_Depth;
    bindingLayoutEntries[6].texture.viewDimension = WGPUTextureViewDimension::WGPUTextureViewDimension_2D;

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

std::vector<WGPUBindGroupLayout> Isometric::OnBindGroupLayoutsBillboard() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(5);

    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[0].buffer.minBindingSize = sizeof(Uniforms);

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[1].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[1].buffer.minBindingSize = sizeof(FrameInfo);

    bindingLayoutEntries[2].binding = 2u;
    bindingLayoutEntries[2].visibility = WGPUShaderStage_Vertex;
    bindingLayoutEntries[2].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_ReadOnlyStorage;
    bindingLayoutEntries[2].buffer.minBindingSize = 10u * sizeof(SpriteInstance);

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

std::vector<WGPUBindGroupLayout> Isometric::OnBindGroupLayoutsShadow() {
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

std::vector<WGPUBindGroupLayout> Isometric::OnBindGroupLayoutsWigglyShadow() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(3);
    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Vertex;
    bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[0].buffer.minBindingSize = sizeof(Uniforms);

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Vertex;
    bindingLayoutEntries[1].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[1].buffer.minBindingSize = sizeof(glm::vec4);

    bindingLayoutEntries[2].binding = 2u;
    bindingLayoutEntries[2].visibility = WGPUShaderStage_Vertex;
    bindingLayoutEntries[2].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_ReadOnlyStorage;
    bindingLayoutEntries[2].buffer.minBindingSize = 1000u * sizeof(glm::mat4);

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

std::vector<WGPUBindGroup> Isometric::OnBindGroups() {
    std::vector<WGPUBindGroup> bindGroups(1);

    std::vector<WGPUBindGroupEntry> bindGroupEntries(4);
    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].buffer = m_uniformBuffer.getBuffer();
    bindGroupEntries[0].offset = 0u;
    bindGroupEntries[0].size = sizeof(Uniforms);

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].buffer = m_skinBuffer.getBuffer();
    bindGroupEntries[1].offset = 0u;
    bindGroupEntries[1].size = wgpuBufferGetSize(m_skinBuffer.getBuffer());

    bindGroupEntries[2].binding = 2u;
    bindGroupEntries[2].sampler = wgpContext.getSampler(SS_0);

    bindGroupEntries[3].binding = 3u;
    bindGroupEntries[3].textureView = m_wgpTextureShadow.getTextureView();

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_ANIMATION"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    bindGroups[0] = wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);

    return bindGroups;
}

std::vector<WGPUBindGroup> Isometric::OnBindGroupsFloor() {
    std::vector<WGPUBindGroup> bindGroups(1);

    std::vector<WGPUBindGroupEntry> bindGroupEntries(5);
    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].buffer = m_uniformBuffer.getBuffer();
    bindGroupEntries[0].offset = 0u;
    bindGroupEntries[0].size = sizeof(Uniforms);

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].sampler = wgpContext.getSampler(SS_LINEAR_REPEAT);

    bindGroupEntries[2].binding = 2u;
    bindGroupEntries[2].textureView = m_wgpFloorD.getTextureView();

    bindGroupEntries[3].binding = 3u;
    bindGroupEntries[3].sampler = wgpContext.getSampler(SS_0);

    bindGroupEntries[4].binding = 4u;
    bindGroupEntries[4].textureView = m_wgpTextureShadow.getTextureView();

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_FLOOR"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    bindGroups[0] = wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);

    return bindGroups;
}

std::vector<WGPUBindGroup> Isometric::OnBindGroupsShadow() {
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
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_PLAYER_SHADOW"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    bindGroups[0] = wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);

    return bindGroups;
}

WGPUBindGroup Isometric::createBindGroupWiggly() {
    std::vector<WGPUBindGroupEntry> bindGroupEntries(7);
    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].buffer = m_uniformBuffer.getBuffer();
    bindGroupEntries[0].offset = 0u;
    bindGroupEntries[0].size = wgpuBufferGetSize(m_uniformBuffer.getBuffer());

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].buffer = m_wigglyBuffer.getBuffer();
    bindGroupEntries[1].offset = 0u;
    bindGroupEntries[1].size = sizeof(glm::vec4);

    bindGroupEntries[2].binding = 2u;
    bindGroupEntries[2].buffer = m_storageBuffer.getBuffer();
    bindGroupEntries[2].offset = 0u;
    bindGroupEntries[2].size = wgpuBufferGetSize(m_storageBuffer.getBuffer());

    bindGroupEntries[3].binding = 3u;
    bindGroupEntries[3].sampler = wgpContext.getSampler(SS_LINEAR_REPEAT);

    bindGroupEntries[4].binding = 4u;
    bindGroupEntries[4].textureView = m_wgpEnemyD.getTextureView();

    bindGroupEntries[5].binding = 5u;
    bindGroupEntries[5].sampler = wgpContext.getSampler(SS_0);

    bindGroupEntries[6].binding = 6u;
    bindGroupEntries[6].textureView = m_wgpTextureShadow.getTextureView();

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_WIGGLY"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    return wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
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

WGPUBindGroup Isometric::CreateBindGroupShadow(const WgpBuffer& uniformBuffer, const WgpBuffer& wigglyBuffer, const WgpBuffer& storageBuffer) {
    std::vector<WGPUBindGroupEntry> bindGroupEntries(3);
    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].buffer = uniformBuffer.getBuffer();
    bindGroupEntries[0].offset = 0u;
    bindGroupEntries[0].size = wgpuBufferGetSize(uniformBuffer.getBuffer());

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].buffer = wigglyBuffer.getBuffer();
    bindGroupEntries[1].offset = 0u;
    bindGroupEntries[1].size = sizeof(glm::vec4);

    bindGroupEntries[2].binding = 2u;
    bindGroupEntries[2].buffer = storageBuffer.getBuffer();
    bindGroupEntries[2].offset = 0u;
    bindGroupEntries[2].size = wgpuBufferGetSize(storageBuffer.getBuffer());

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_WIGGLY_SHADOW"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    return wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
}

WGPUBindGroup Isometric::createBindGroupBillboard() {
    std::vector<WGPUBindGroupEntry> bindGroupEntries(5);

    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].buffer = m_uniformBuffer.getBuffer();
    bindGroupEntries[0].offset = 0u;
    bindGroupEntries[0].size = wgpuBufferGetSize(m_uniformBuffer.getBuffer());

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].buffer = m_infoBufferBillboard.getBuffer();
    bindGroupEntries[1].offset = 0u;
    bindGroupEntries[1].size = wgpuBufferGetSize(m_infoBufferBillboard.getBuffer());

    bindGroupEntries[2].binding = 2u;
    bindGroupEntries[2].buffer = m_spriteBuffer.getBuffer();
    bindGroupEntries[2].offset = 0u;
    bindGroupEntries[2].size = wgpuBufferGetSize(m_spriteBuffer.getBuffer());

    bindGroupEntries[3].binding = 3u;
    bindGroupEntries[3].sampler = wgpContext.getSampler(SS_LINEAR_CLAMP);

    bindGroupEntries[4].binding = 4u;
    bindGroupEntries[4].textureView = m_sprite.getTextureView();

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_BILLBOARD"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = (WGPUBindGroupEntry*)bindGroupEntries.data();
    return wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
}

WGPUBindGroup Isometric::createBindGroupMuzzle() {
    std::vector<WGPUBindGroupEntry> bindGroupEntries(5);

    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].buffer = m_uniformBuffer.getBuffer();
    bindGroupEntries[0].offset = 0u;
    bindGroupEntries[0].size = wgpuBufferGetSize(m_uniformBuffer.getBuffer());

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].buffer = m_infoBufferMuzzle.getBuffer();
    bindGroupEntries[1].offset = 0u;
    bindGroupEntries[1].size = wgpuBufferGetSize(m_infoBufferMuzzle.getBuffer());

    bindGroupEntries[2].binding = 2u;
    bindGroupEntries[2].buffer = m_muzzleBuffer.getBuffer();
    bindGroupEntries[2].offset = 0u;
    bindGroupEntries[2].size = wgpuBufferGetSize(m_muzzleBuffer.getBuffer());

    bindGroupEntries[3].binding = 3u;
    bindGroupEntries[3].sampler = wgpContext.getSampler(SS_LINEAR_CLAMP);

    bindGroupEntries[4].binding = 4u;
    bindGroupEntries[4].textureView = m_muzzle.getTextureView();

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_MUZZLE"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = (WGPUBindGroupEntry*)bindGroupEntries.data();
    return wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
}

float Isometric::getLookAtYRotation(const glm::vec3& objectPos, const glm::vec3& targetPos) {
    float dx = targetPos[0] - objectPos[0];
    float dz = targetPos[2] - objectPos[2];

    if (abs(dx) < 0.01f && abs(dz) < 0.01f)
        return 0.0f;

    return glm::degrees(std::atan2(dx, dz));
}

CollisionEntity* Isometric::createNewBulletToPool() {
    btTransform startTransform;
    startTransform.setIdentity();

    btCollisionObject* body = Physics::AddKinematicObject(startTransform, new btCapsuleShapeX(0.03f, 0.3f), Physics::collisiontypes::SPHERE, Physics::collisiontypes::ENEMY);

    CollisionEntity* entity = m_scene->addChildSilent<CollisionEntity>(body);
    entity->setActive(false);
    m_entities.push_back(entity);

    return entity;
}

void Isometric::spawnBillboard(const glm::vec3& position) {
    SpriteInstance billboard;
    billboard.position[0] = position[0];
    billboard.position[1] = 120.0f * 0.0044f;
    billboard.position[2] = position[2];

    billboard.scale[0] = 0.25f;
    billboard.scale[1] = 0.25f;
    billboard.age = 0.0f;
    billboard.currentFrame = 0u;

    m_activeBillboards.push_back(billboard);
}

void Isometric::resetMuzzle() {
    if (m_muzzleInstance.currentFrame >= 6u) {
        m_muzzleInstance.position[0] = 0.0f;
        m_muzzleInstance.position[1] = 0.0f;
        m_muzzleInstance.position[2] = 0.0f;
        m_muzzleInstance.scale[0] = 1.0f;
        m_muzzleInstance.scale[1] = 1.0f;
        m_muzzleInstance.currentFrame = 0u;
        m_muzzleInstance.age = 0.0f;
    }
}

void Isometric::updateBillboards(float dt) {
    const uint32_t numCols = 11;
    const float timePerSprite = 0.05f;
    const float spritesheetDur = numCols * timePerSprite;

    for (auto& sprite : m_activeBillboards) {
        sprite.age += dt;
        sprite.currentFrame = sprite.age / timePerSprite;
    }

    m_activeBillboards.erase(
            std::remove_if(
                    m_activeBillboards.begin(),
                    m_activeBillboards.end(),
                    [spritesheetDur](const SpriteInstance& s) {
                        return s.age >= spritesheetDur;
                    }
            ),
            m_activeBillboards.end()
    );
}

void Isometric::updateMuzzle(float dt) {
    if (m_muzzleInstance.currentFrame < 6u) {
        const uint32_t numCols = 6;
        const float timePerSprite = 0.05f;
        const float spritesheetDur = numCols * timePerSprite;

        m_muzzleInstance.age += dt;
        m_muzzleInstance.currentFrame = m_muzzleInstance.age / timePerSprite;
    }
}

void Isometric::OnDrawShadow(const WGPURenderPassEncoder& renderPassEncoder) {
    wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_PLAYER_SHADOW"));
    m_wgpPlayer.setBindGroupsSlot("SHADOW");
    m_wgpPlayer.draw(renderPassEncoder);

    wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_WIGGLY_SHADOW"));
    m_wgpEnemy.setBindGroupsSlot("SHADOW");
    m_wgpEnemy.draw(renderPassEncoder, m_cpuInstanceBuffer.size());


    m_wgpEnemy.setBindGroupsSlot("BG");
    m_wgpPlayer.setBindGroupsSlot("BG");
}