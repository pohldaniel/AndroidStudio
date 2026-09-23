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
    wgpSetSurfaceDepthFormat(WGPUTextureFormat::WGPUTextureFormat_Depth24Plus);

    nkInit(static_cast<float>(wgpWidth), static_cast<float>(wgpHeight));
    nkInitFont("fonts/upheavtt.ttf");

    m_camera.perspective(glm::radians(45.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 100.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), 0.0f, static_cast<float>(wgpHeight), -1.0f, 1.0f);
    m_camera.lookAt(glm::vec3(0.0f, 4.3f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_camera.setMovingSpeed(50.0f);
    m_camera.setRotationSpeed(0.1f);

    m_trackball.reshape(wgpWidth, wgpHeight);
    m_floor.buildQuadXZ({-50.0f, 0.0f, -50.0f}, {100.0f, 100.0f});
    m_floor.rotate(0.0f, 45.0f, 0.0f);
    m_bullet.buildQuadXZ({ -0.3f * 0.243f, 0.0f, -0.3f * 0.243f }, { 0.3f * 0.5f, 0.3f * 0.5f }, 1u, 1u, true, false);

    AnimationManager::Get().getAnimation("full").loadAnimationAssimp("models/player/Player.fbx", "Player", "full", 0u, 245u);
    AnimationManager::Get().getAnimation("idle").loadAnimationAssimp("models/player/Player.fbx", "Player", "idle", 5u, 81u);
    AnimationManager::Get().getAnimation("forward").loadAnimationAssimp("models/player/Player.fbx", "Player", "forward", 85u, 105u);
    AnimationManager::Get().getAnimation("backward").loadAnimationAssimp("models/player/Player.fbx", "Player", "backward", 110u, 130u);
    AnimationManager::Get().getAnimation("backward").shift(10u);
    AnimationManager::Get().getAnimation("right").loadAnimationAssimp("models/player/Player.fbx", "Player", "right", 135u, 155u);
    AnimationManager::Get().getAnimation("right").shift(10u);
    AnimationManager::Get().getAnimation("left").loadAnimationAssimp("models/player/Player.fbx", "Player", "left", 160u, 180u);
    AnimationManager::Get().getAnimation("death").loadAnimationAssimp("models/player/Player.fbx", "Player", "death", 185u, 244u);

    m_player.loadModelAssimp("models/player/Player.fbx", 1u);
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
    m_rotationBuffer.createBuffer(sizeof(glm::vec4) * 400000u, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    m_offsetBuffer.createBuffer(sizeof(glm::vec4) * 400000u, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);

    m_spriteBuffer.createBuffer(20u * sizeof(SpriteInstance), WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    m_muzzleBuffer.createBuffer(100u * sizeof(SpriteInstance), WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);

    m_directionalLightBuffer.createBuffer(sizeof(DirectionalLight), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);
    m_pointLightBuffer.createBuffer(sizeof(PointLight), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);

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

    DirectionalLight directional;
    directional.color[0] = 0.35f * 0.7f * 0.406f; directional.color[1] = 0.35f * 0.7f * 0.723f; directional.color[2] = 0.35f * 0.7f; directional.color[3] = 1.0f;
    directional.padding = 0.0f;

    glm::vec3 dir = glm::normalize(glm::vec3(-0.8f, 0.0f, -1.0f));
    directional.direction[0] = dir[0];
    directional.direction[1] = dir[1];
    directional.direction[2] = dir[2];
    wgpuQueueWriteBuffer(wgpContext.queue, m_directionalLightBuffer.getBuffer(), 0u, &directional, sizeof(DirectionalLight));


    m_wgpBulletTexture.setFlipHorizontal(true);
    m_wgpBulletTexture.loadFromFile("textures/BulletTexture.png");
    m_sprite.loadFromFile("textures/impact_spritesheet_with_00.png");
    m_muzzle.loadFromFile("textures/muzzle_spritesheet.png");

    m_wgpPlayerD.loadFromFile("models/player/Player_D.tga", true);
    m_wgpPlayerN.loadFromFile("models/player/Player_N.tga", true);
    m_wgpPlayerS.loadFromFile("models/player/Player_S.tga", true);
    m_wgpPlayerE.loadFromFile("models/player/Player_E.tga", true);

    m_wgpGunD.loadFromFile("models/player/Gun_D.tga", true);
    m_wgpGunN.loadFromFile("models/player/Gun_N.tga", true);
    m_wgpGunS.loadFromFile("models/player/Gun_S.tga", true);
    m_wgpGunE.loadFromFile("models/player/Gun_E.tga", true);

    m_wgpFloorD.loadFromFile("textures/floor/Floor_D.psd");
    m_wgpFloorN.loadFromFile("textures/floor/Floor_N.psd");
    m_wgpFloorS.loadFromFile("textures/floor/Floor_S.psd");

    m_wgpEnemyD.loadFromFile("models/EelDog/Eeldog_Albedo.tif");
    m_wgpEnemyN.loadFromFile("models/EelDog/Eeldog_Normal.tif");
    m_wgpEnemyS.loadFromFile("models/EelDog/Eeldog_MetallicSmooth.tif");

    m_wgpTextureShadow.createEmpty(4u * 1024u, 4u * 1024u, 1u, WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment, WGPUTextureFormat_Depth32Float);

    m_wgpEmissionTarget.createEmpty(wgpWidth, wgpHeight, 1u, WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment, WGPUTextureFormat_RGBA8Unorm, 1u, 1u);
    m_wgpEmissionDepth.createEmpty(wgpWidth, wgpHeight, 1u, WGPUTextureUsage_RenderAttachment, WGPUTextureFormat_Depth16Unorm, 1u, 1u);

    m_wgpSceneTarget.createEmpty(wgpWidth, wgpHeight, 1u, WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment, WGPUTextureFormat_RGBA8Unorm, 1u, 4u);
    m_wgpSceneDepth.createEmpty(wgpWidth, wgpHeight, 1u, WGPUTextureUsage_RenderAttachment, WGPUTextureFormat_Depth24Plus, 1u, 4u);

    m_wgpBlurTempTarget.createEmpty(wgpWidth / 2, wgpHeight / 2, 1u, WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment, WGPUTextureFormat_RGBA8Unorm, 1u, 1u);
    m_wgpBlurFinalTarget.createEmpty(wgpWidth / 2, wgpHeight / 2, 1u, WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment, WGPUTextureFormat_RGBA8Unorm, 1u, 1u);

    wgpContext.addSampler(wgpCreateSampler(WGPUFilterMode_Linear, WGPUAddressMode_ClampToEdge, 1u, WGPUMipmapFilterMode_Nearest, WGPUCompareFunction_Greater), SS_0);
    wgpContext.addSahderModule("COMPOSITE", "shader/composite.wgsl");
    wgpContext.createRenderPipeline("COMPOSITE", "RP_COMPOSITE", VL_NONE, std::bind(&Isometric::OnBindGroupLayoutsComposite, this), 1u);

    wgpContext.addSahderModule("ANIMATION", "shader/player.wgsl");
    wgpContext.createRenderPipeline("ANIMATION", "RP_ANIMATION", VL_PTNWJ, std::bind(&Isometric::OnBindGroupLayouts, this), 4u);

    wgpContext.addSahderModule("EMISSION", "shader/player_emission.wgsl");
    wgpContext.createRenderPipeline("EMISSION", "RP_PLAYER_EMISSION", VL_PTNWJ, std::bind(&Isometric::OnBindGroupLayoutsEmission, this),
                                    1u, WGPUPrimitiveTopology_TriangleList, WGPUTextureFormat_Undefined, WGPUTextureFormat_Depth16Unorm);

    wgpContext.addSahderModule("FLOOR", "shader/floor.wgsl");
    wgpContext.createRenderPipeline("FLOOR", "RP_FLOOR", VL_PTN, std::bind(&Isometric::OnBindGroupLayoutsFloor, this), 4u);

    wgpContext.addSahderModule("FLOOR_MASK", "shader/floor_emission.wgsl");
    wgpContext.createRenderPipeline("FLOOR_MASK", "RP_FLOOR_EMISSION", VL_PTN, std::bind(&Isometric::OnBindGroupLayoutsFloorEmission, this),
                                    1u, WGPUPrimitiveTopology_TriangleList, WGPUTextureFormat_Undefined, WGPUTextureFormat_Depth16Unorm, WGPUCompareFunction_Always,
                                    { DEPTH_STENCIL_STATE | FRAGMENT_STATE, ColorMode::WRITE_NONE });

    wgpContext.addSahderModule("WIGGLY", "shader/wiggly.wgsl");
    wgpContext.createRenderPipeline("WIGGLY", "RP_WIGGLY", VL_PTN, std::bind(&Isometric::OnBindGroupLayoutsWiggly, this), 4u);

    wgpContext.addSahderModule("BULLET", "shader/bullet.wgsl");
    wgpContext.createRenderPipeline("BULLET", "RP_BULLET", VL_PT, std::bind(&Isometric::OnBindGroupLayoutsBullet, this),
                                    4u, WGPUPrimitiveTopology_TriangleList, WGPUTextureFormat_Undefined, WGPUTextureFormat_Undefined, WGPUCompareFunction_Always,
                                    { DEPTH_STENCIL_STATE | BLEND_STATE | FRAGMENT_STATE, ColorMode::WRITE_RGBA, DepthMode::WRITE, StencilMode::DEFAULT, BlendMode::ALPHA_BLENDING, WGPUTextureFormat_Undefined, WGPUCullMode_None });

    wgpContext.createRenderPipeline("BULLET", "RP_BULLET_EMISSION", VL_PT, std::bind(&Isometric::OnBindGroupLayoutsBullet, this),
                                    1u, WGPUPrimitiveTopology_TriangleList, WGPUTextureFormat_Undefined, WGPUTextureFormat_Depth16Unorm, WGPUCompareFunction_LessEqual,
                                    { DEPTH_STENCIL_STATE | BLEND_STATE | FRAGMENT_STATE, ColorMode::WRITE_RGBA, DepthMode::WRITE, StencilMode::DEFAULT, BlendMode::ALPHA_BLENDING, WGPUTextureFormat_Undefined, WGPUCullMode_None });

    wgpContext.addSahderModule("BILLBOARD", "shader/billboard.wgsl");
    wgpContext.createRenderPipeline("BILLBOARD", "RP_BILLBOARD", VL_NONE, std::bind(&Isometric::OnBindGroupLayoutsBillboard, this),
                                    4u, WGPUPrimitiveTopology_TriangleStrip, WGPUTextureFormat_Undefined, WGPUTextureFormat_Undefined, WGPUCompareFunction_Always,
                                    { DEPTH_STENCIL_STATE | BLEND_STATE | FRAGMENT_STATE });

    wgpContext.addSahderModule("MUZZLE", "shader/muzzle.wgsl");
    wgpContext.createRenderPipeline("MUZZLE", "RP_MUZZLE", VL_NONE, std::bind(&Isometric::OnBindGroupLayoutsBillboard, this),
                                    4u, WGPUPrimitiveTopology_TriangleStrip, WGPUTextureFormat_Undefined, WGPUTextureFormat_Undefined, WGPUCompareFunction_Always,
                                    { DEPTH_STENCIL_STATE | BLEND_STATE | FRAGMENT_STATE, ColorMode::WRITE_RGBA, DepthMode::WRITE, StencilMode::DEFAULT, BlendMode::ALPHA_BLENDING, WGPUTextureFormat_Undefined, WGPUCullMode_None });

    wgpContext.addSahderModule("PLAYER_SHADOW", "shader/player_shadow.wgsl");
    wgpContext.createRenderPipeline("PLAYER_SHADOW", "RP_PLAYER_SHADOW", VL_PTNWJ, std::bind(&Isometric::OnBindGroupLayoutsShadow, this),
                                    1u, WGPUPrimitiveTopology_TriangleList, WGPUTextureFormat_Undefined, WGPUTextureFormat_Depth32Float, WGPUCompareFunction_Less,
                                    { DEPTH_STENCIL_STATE }
    );

    wgpContext.addSahderModule("WIGGLY_SHADOW", "shader/wiggly_shadow.wgsl");
    wgpContext.createRenderPipeline("WIGGLY_SHADOW", "RP_WIGGLY_SHADOW", VL_PTN, std::bind(&Isometric::OnBindGroupLayoutsWigglyShadow, this),
                                    1u, WGPUPrimitiveTopology_TriangleList, WGPUTextureFormat_Undefined, WGPUTextureFormat_Depth32Float, WGPUCompareFunction_Less,
                                    { DEPTH_STENCIL_STATE}
    );

    wgpContext.addSahderModule("BLUR_HORIZONTAL", "shader/blur_horizontal.wgsl");
    wgpContext.createRenderPipeline("BLUR_HORIZONTAL", "RP_BLUR_HORIZONTAL", VL_NONE, std::bind(&Isometric::OnBindGroupLayoutsBlur, this),
                                    1u, WGPUPrimitiveTopology_TriangleList, WGPUTextureFormat_Undefined, WGPUTextureFormat_Undefined, WGPUCompareFunction_Always,
                                    { FRAGMENT_STATE}
    );

    wgpContext.addSahderModule("BLUR_VERTICAL", "shader/blur_vertical.wgsl");
    wgpContext.createRenderPipeline("BLUR_VERTICAL", "RP_BLUR_VERTICAL", VL_NONE, std::bind(&Isometric::OnBindGroupLayoutsBlur, this),
                                    1u, WGPUPrimitiveTopology_TriangleList, WGPUTextureFormat_Undefined, WGPUTextureFormat_Undefined, WGPUCompareFunction_Always,
                                    { FRAGMENT_STATE }
    );

    m_wgpPlayer.create(m_player);
    m_wgpPlayer.setBindGroups("SHADOW", std::bind(&Isometric::OnBindGroupsShadow, this));
    m_wgpPlayer.getMesh(0u).setBindGroups("EMISSION", std::bind(&Isometric::OnBindGroupsPlayerEmission, this));
    m_wgpPlayer.getMesh(0u).setBindGroups("BG", std::bind(&Isometric::OnBindGroupsPlayer, this));
    m_wgpPlayer.getMesh(1u).setBindGroups("EMISSION", std::bind(&Isometric::OnBindGroupsGunEmission, this));
    m_wgpPlayer.getMesh(1u).setBindGroups("BG", std::bind(&Isometric::OnBindGroupsGun, this));

    m_wgpFloor.create(m_floor);
    m_wgpFloor.addBindGroups("EMISSION", std::bind(&Isometric::OnBindGroupsFloorEmission, this));
    m_wgpFloor.setBindGroups("BG", std::bind(&Isometric::OnBindGroupsFloor, this));

    m_wgpEnemy.create(m_enemy);
    m_wgpEnemy.addBindGroup("SHADOW", CreateBindGroupShadow(m_uniformBuffer, m_wigglyBuffer, m_storageBuffer));
    m_wgpEnemy.addBindGroup("BG", createBindGroupWiggly());

    m_wgpBullet.create(m_bullet);
    m_wgpBullet.setBindGroups("BG", std::bind(&Isometric::OnBindGroupsBullet, this));

    wgpContext.setClearColor({ 0.2f, 0.2f, 0.2f, 1.0f });
    wgpContext.OnDraw = std::bind(&Isometric::OnDraw, this, std::placeholders::_1, std::placeholders::_2);
    wgpContext.OnPostDraw = std::bind(&Isometric::OnPostDraw, this);
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

    btCollisionObject* body = Physics::AddKinematicObject(Physics::BtTransform(glm::vec3(0.0f, 0.4f, 0.0f)), new btCylinderShape(btVector3(0.35f * 0.5f, 0.4f, 0.35f * 0.5f)), Physics::collisiontypes::CHARACTER, Physics::collisiontypes::ENEMY);
    m_playerEntity = m_scene->addChild<Player>(body, m_player);

    m_bindGroupBillboard = createBindGroupBillboard();
    m_bindGroupMuzzle = createBindGroupMuzzle();
    m_bindGroupComposite = createBindGroupComposite();
    m_bindGroupBlurH = createBindGroupBlurH();
    m_bindGroupBlurV = createBindGroupBlurV();
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
    m_pointLightBuffer.markForDelete();
    m_directionalLightBuffer.markForDelete();

    m_wgpBulletTexture.markForDelete();
    m_sprite.markForDelete();
    m_muzzle.markForDelete();

    m_wgpFloorD.markForDelete();
    m_wgpFloorN.markForDelete();
    m_wgpFloorS.markForDelete();
    m_wgpEnemyD.markForDelete();
    m_wgpEnemyN.markForDelete();
    m_wgpEnemyS.markForDelete();
    m_wgpPlayerD.markForDelete();
    m_wgpPlayerN.markForDelete();
    m_wgpPlayerS.markForDelete();
    m_wgpPlayerE.markForDelete();
    m_wgpGunD.markForDelete();
    m_wgpGunN.markForDelete();
    m_wgpGunS.markForDelete();
    m_wgpGunE.markForDelete();

    m_wgpTextureShadow.markForDelete();
    m_wgpEmissionTarget.markForDelete();
    m_wgpEmissionDepth.markForDelete();
    m_wgpSceneTarget.markForDelete();
    m_wgpSceneDepth.markForDelete();
    m_wgpBlurTempTarget.markForDelete();
    m_wgpBlurFinalTarget.markForDelete();

    wgpuBindGroupRelease(m_bindGroupBillboard);
    wgpuBindGroupRelease(m_bindGroupMuzzle);

    wgpuBindGroupRelease(m_bindGroupComposite);
    wgpuBindGroupRelease(m_bindGroupBlurH);
    wgpuBindGroupRelease(m_bindGroupBlurV);
}

void Isometric::fixedUpdate() {
    for (auto enemy : m_enemies) {
        enemy->fixedUpdate(m_fdt);
    }

    m_playerEntity->fixedUpdate(m_fdt);

    Globals::physics->stepSimulation(FIXED_STEP);

    BulletCollisionCallback callback;
    Physics::GetDynamicsWorld()->contactTest(m_playerEntity->getCollisionObject(), callback);
    if (callback.m_hasCollided && callback.m_hitTarget) {
        m_playerEntity->setActive(false);
        m_isDeath = true;
    }
}

void Isometric::update() {
	nkUpdateInput(0, 0, false, false, 0.0f);

    if (!m_isDeath && m_rotationButtonResult.buttonDown && (lastFireTime + 0.1f) < Clock()) {
        const glm::quat midOri = m_player.getOrientation();
        const glm::mat4 playerModelTransform = m_player.getWorldTransformation();
        const glm::vec3 projectileSpawnPoint = playerModelTransform * glm::vec4(-20.0f, 120.0f, 100.0f, 1.0f);

        m_bulletStore.createBullets(projectileSpawnPoint, midOri, m_spreadAmount);
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
        m_playerEntity->translate(playerDirection[0] * 2.0f * m_dt, playerDirection[1] * 2.0f * m_dt, playerDirection[2] * 2.0f * m_dt);
        m_lightView = glm::lookAt(m_playerEntity->getPosition() - 20.0f * m_lightDir, m_playerEntity->getPosition(), glm::vec3(0.0f, 1.0f, 0.0f));
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
    m_bulletStore.updateBullets(m_dt, m_enemies);
    m_enemySpawner.update(posistion, m_dt);
    for (auto enemy : m_enemies) {
        enemy->update(m_dt);
    }
    updateBillboards(m_dt);

    const AnimatedMesh* mesh = static_cast<const AnimatedMesh*>(m_player.getMesh());
    mesh->skinMatrices()[42] = mesh->getBone(43u).getWorldTransformation() * offset * pivot * mesh->skinMatrices()[42];
    wgpuQueueWriteBuffer(wgpContext.queue, m_skinBuffer.getBuffer(), 0u, mesh->getSkinMatrices(), mesh->getNumBones() * sizeof(glm::mat4));

    glm::mat4 muzzleTransform = mesh->skinMatrices()[42] * glm::translate(glm::vec3(214.0f, 76.143f, -3.054f));
    updateMuzzle(m_dt, muzzleTransform[3][0], muzzleTransform[3][1], muzzleTransform[3][2]);

    float angle = aimTheta;
    while (angle < 0.0f) angle += 360.0f;
    while (angle >= 360.0f) angle -= 360.0f;
    float radians = glm::radians(angle);
    float finalCorrectionAngle = 90.0f * std::abs(std::cos(radians));
    float sign = (angle > 0.0f && angle < 180.0f) ? -1.0f : 1.0f;

    muzzleTransform = muzzleTransform * glm::scale(glm::vec3(100.0f, 100.0f, 100.0f)) * glm::rotate(glm::radians(sign * finalCorrectionAngle), glm::vec3(1.0f, 0.0f, 0.0f));

    m_uniforms.projection = m_camera.getPerspectiveMatrix();
    m_uniforms.view = m_camera.getViewMatrix();
    m_uniforms.env = m_camera.getRotationMatrix();
    m_uniforms.model = muzzleTransform;
    m_uniforms.normal = Camera::GetNormalMatrix(m_camera.getViewMatrix() * m_uniforms.model);
    m_uniforms.camPosition = m_camera.getPosition();
    m_uniforms.lightVP = m_lightProjection * m_lightView;
    m_uniforms.shadow = Camera::BIAS * m_uniforms.lightVP;
    m_uniforms.lightPosition = m_playerEntity->getPosition() - 20.0f * m_lightDir;

    wgpuQueueWriteBuffer(wgpContext.queue, m_uniformBuffer.getBuffer(), 0, &m_uniforms, sizeof(Uniforms));

    m_cpuInstanceBuffer.clear();
    m_cpuInstanceBuffer.reserve(m_enemies.size());

    for (const auto& enemy : m_enemies) {
        if (enemy->isDeath()) {
            enemy->setActive(false);
            m_scene->eraseChild(enemy);
            m_ding.play("sounds/bullet_hit_metal_enemy_4.wav");
            spawnBillboard(enemy->getPosition());
            continue;
        }
        m_cpuInstanceBuffer.push_back(enemy->getTransformationSOP());
    }

    wgpuQueueWriteBuffer(wgpContext.queue, m_storageBuffer.getBuffer(), 0u, m_cpuInstanceBuffer.data(), m_cpuInstanceBuffer.size() * sizeof(glm::mat4));

    m_wiggly.nosePos[0] = 1.0f ;
    m_wiggly.nosePos[1] = 120.0f * 0.0044f ;
    m_wiggly.nosePos[2] = -2.0f ;
    m_wiggly.time = Clock();
    wgpuQueueWriteBuffer(wgpContext.queue, m_wigglyBuffer.getBuffer(), 0, &m_wiggly, sizeof(Wiggly));

    wgpuQueueWriteBuffer(wgpContext.queue, m_rotationBuffer.getBuffer(), 0u, m_bulletStore.m_rots.data(), m_bulletStore.m_rots.size() * sizeof(glm::vec4));
    wgpuQueueWriteBuffer(wgpContext.queue, m_offsetBuffer.getBuffer(), 0u, m_bulletStore.m_offsets.data(), m_bulletStore.m_offsets.size() * sizeof(glm::vec4));

    wgpuQueueWriteBuffer(wgpContext.queue, m_spriteBuffer.getBuffer(), 0u, m_activeBillboards.data(), m_activeBillboards.size() * sizeof(SpriteInstance));
    wgpuQueueWriteBuffer(wgpContext.queue, m_muzzleBuffer.getBuffer(), 0u, &m_muzzleInstance, sizeof(SpriteInstance));
}

void Isometric::render() {
    wgpDraw();
}

void Isometric::OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor) {

    {
        WgpRenderer::Draw(m_wgpEmissionTarget, m_wgpEmissionDepth, std::bind(&Isometric::OnDrawEmission, this, std::placeholders::_1));
    }

    {
        WgpRenderer::DrawDepth(m_wgpTextureShadow, std::bind(&Isometric::OnDrawShadow, this, std::placeholders::_1));
    }

    {
        WgpRenderer::Draw(m_wgpSceneTarget, m_wgpSceneDepth, std::bind(&Isometric::OnDrawScene, this, std::placeholders::_1));
    }

    {
        WgpRenderer::DrawColor(m_wgpBlurTempTarget, [this](const WGPURenderPassEncoder& renderPassEncoder) {
            wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_BLUR_HORIZONTAL"));
            wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0u, m_bindGroupBlurH, 0u, nullptr);
            wgpuRenderPassEncoderDraw(renderPassEncoder, 6u, 1u, 0u, 0u);
        });
    }

    {
        WgpRenderer::DrawColor(m_wgpBlurFinalTarget, [this](const WGPURenderPassEncoder& renderPassEncoder) {
            wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_BLUR_VERTICAL"));
            wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0u, m_bindGroupBlurV, 0u, nullptr);
            wgpuRenderPassEncoderDraw(renderPassEncoder, 6u, 1u, 0u, 0u);
        });
    }

    {
        WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);
        wgpuRenderPassEncoderSetViewport(renderPassEncoder, 0.0f, 0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f, 1.0f);

        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_COMPOSITE"));
        wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0u, m_bindGroupComposite, 0u, nullptr);
        wgpuRenderPassEncoderDraw(renderPassEncoder, 6u, 1u, 0u, 0u);

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

void Isometric::OnPostDraw() {
    if (m_wantResize) {

        m_wgpEmissionTarget.resize(wgpWidth, wgpHeight);
        m_wgpEmissionDepth.resize(wgpWidth, wgpHeight);

        m_wgpSceneTarget.resize(wgpWidth, wgpHeight);
        m_wgpSceneDepth.resize(wgpWidth, wgpHeight);

        m_wgpBlurTempTarget.resize(wgpWidth / 2, wgpHeight / 2);
        m_wgpBlurFinalTarget.resize(wgpWidth / 2, wgpHeight / 2);

        wgpuBindGroupRelease(m_bindGroupComposite);
        wgpuBindGroupRelease(m_bindGroupBlurH);
        wgpuBindGroupRelease(m_bindGroupBlurV);

        m_bindGroupComposite = createBindGroupComposite();
        m_bindGroupBlurH = createBindGroupBlurH();
        m_bindGroupBlurV = createBindGroupBlurV();

        m_wantResize = false;
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
    m_trackball.reshape(static_cast<int>(wgpWidth), static_cast<int>(wgpHeight));
    m_wantResize = true;
}

void Isometric::OnButton(const Event::MouseButtonEvent& event) {
    AnimationManager::Get().clear();
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
    SpriteInstance billboard = {};
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

void Isometric::updateMuzzle(float dt, float x, float y, float z) {
    const uint32_t numCols = 6;
    const float timePerSprite = 0.05f;

    if (!m_muzzleFlashSpritesAge.empty()) {
        for (float & i : m_muzzleFlashSpritesAge) {
            i += dt;
        }

        const float maxAge = numCols * timePerSprite;
        m_muzzleFlashSpritesAge.erase(
                std::remove_if(m_muzzleFlashSpritesAge.begin(), m_muzzleFlashSpritesAge.end(),
                               [maxAge](const float f) { return f >= maxAge; }),
                m_muzzleFlashSpritesAge.end()
        );
    }

    float minAge = 1000.0f;
    for (const float a : m_muzzleFlashSpritesAge) {
        minAge = std::min(a, minAge);
    }

    PointLight point = {};
    point.color[0] = 1.0f; point.color[1] = 0.2f; point.color[2] = 0.0f; point.color[3] = 1.0f;
    point.padding = 0.0f;
    point.position[0] = x;
    point.position[1] = y;
    point.position[2] = z;
    point.active = (minAge < 0.03f) ? 1u : 0u;

    wgpuQueueWriteBuffer(wgpContext.queue, m_pointLightBuffer.getBuffer(), 0u, &point, sizeof(PointLight));

    if (m_muzzleInstance.currentFrame < 6u) {
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

void Isometric::OnDrawEmission(const WGPURenderPassEncoder& renderPassEncoder) {
    wgpContext.setClearColor({ 0.0f, 0.0f, 0.0f, 0.0f });

    wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_PLAYER_EMISSION"));
    m_wgpPlayer.setBindGroupsSlot("EMISSION");
    m_wgpPlayer.draw(renderPassEncoder);

    wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_FLOOR_EMISSION"));
    m_wgpFloor.setBindGroupsSlot("EMISSION");
    m_wgpFloor.draw(renderPassEncoder);

    wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_BULLET_EMISSION"));
    m_wgpBullet.draw(renderPassEncoder, m_bulletStore.m_rots.size());

    m_wgpFloor.setBindGroupsSlot("BG");
    wgpContext.setClearColor({ 0.0f, 0.0f, 0.0f, 1.0f });
}

void Isometric::OnDrawScene(const WGPURenderPassEncoder& renderPassEncoder) {
    wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_FLOOR"));
    m_wgpFloor.draw(renderPassEncoder);

    wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_WIGGLY"));
    m_wgpEnemy.draw(renderPassEncoder, m_cpuInstanceBuffer.size());

    wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_ANIMATION"));
    m_wgpPlayer.draw(renderPassEncoder);

    wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_BILLBOARD"));
    wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0u, m_bindGroupBillboard, 0u, nullptr);
    wgpuRenderPassEncoderDraw(renderPassEncoder, 4u, m_activeBillboards.size(), 0u, 0u);

    wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_MUZZLE"));
    wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0u, m_bindGroupMuzzle, 0u, nullptr);
    wgpuRenderPassEncoderDraw(renderPassEncoder, 4u, 1u, 0u, 0u);
}

std::vector<WGPUBindGroupLayout> Isometric::OnBindGroupLayouts() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(9);
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
    bindingLayoutEntries[2].sampler.type = WGPUSamplerBindingType::WGPUSamplerBindingType_Filtering;

    bindingLayoutEntries[3].binding = 3u;
    bindingLayoutEntries[3].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[3].texture.sampleType = WGPUTextureSampleType::WGPUTextureSampleType_Float;
    bindingLayoutEntries[3].texture.viewDimension = WGPUTextureViewDimension::WGPUTextureViewDimension_2D;

    bindingLayoutEntries[4].binding = 4u;
    bindingLayoutEntries[4].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[4].texture.sampleType = WGPUTextureSampleType::WGPUTextureSampleType_Float;
    bindingLayoutEntries[4].texture.viewDimension = WGPUTextureViewDimension::WGPUTextureViewDimension_2D;

    bindingLayoutEntries[5].binding = 5u;
    bindingLayoutEntries[5].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[5].texture.sampleType = WGPUTextureSampleType::WGPUTextureSampleType_Float;
    bindingLayoutEntries[5].texture.viewDimension = WGPUTextureViewDimension::WGPUTextureViewDimension_2D;

    bindingLayoutEntries[6].binding = 6u;
    bindingLayoutEntries[6].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[6].sampler.type = WGPUSamplerBindingType::WGPUSamplerBindingType_Comparison;

    bindingLayoutEntries[7].binding = 7u;
    bindingLayoutEntries[7].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[7].texture.sampleType = WGPUTextureSampleType::WGPUTextureSampleType_Depth;
    bindingLayoutEntries[7].texture.viewDimension = WGPUTextureViewDimension::WGPUTextureViewDimension_2D;

    bindingLayoutEntries[8].binding = 8u;
    bindingLayoutEntries[8].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[8].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[8].buffer.minBindingSize = sizeof(PointLight);

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

std::vector<WGPUBindGroupLayout> Isometric::OnBindGroupLayoutsFloor() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(9);
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
    bindingLayoutEntries[3].texture.sampleType = WGPUTextureSampleType::WGPUTextureSampleType_Float;
    bindingLayoutEntries[3].texture.viewDimension = WGPUTextureViewDimension::WGPUTextureViewDimension_2D;

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

    bindingLayoutEntries[7].binding = 7u;
    bindingLayoutEntries[7].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[7].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[7].buffer.minBindingSize = sizeof(DirectionalLight);

    bindingLayoutEntries[8].binding = 8u;
    bindingLayoutEntries[8].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[8].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[8].buffer.minBindingSize = sizeof(PointLight);

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

std::vector<WGPUBindGroupLayout> Isometric::OnBindGroupLayoutsWiggly() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(10);
    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
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
    bindingLayoutEntries[5].texture.sampleType = WGPUTextureSampleType::WGPUTextureSampleType_Float;
    bindingLayoutEntries[5].texture.viewDimension = WGPUTextureViewDimension::WGPUTextureViewDimension_2D;

    bindingLayoutEntries[6].binding = 6u;
    bindingLayoutEntries[6].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[6].texture.sampleType = WGPUTextureSampleType::WGPUTextureSampleType_Float;
    bindingLayoutEntries[6].texture.viewDimension = WGPUTextureViewDimension::WGPUTextureViewDimension_2D;

    bindingLayoutEntries[7].binding = 7u;
    bindingLayoutEntries[7].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[7].sampler.type = WGPUSamplerBindingType::WGPUSamplerBindingType_Comparison;

    bindingLayoutEntries[8].binding = 8u;
    bindingLayoutEntries[8].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[8].texture.sampleType = WGPUTextureSampleType::WGPUTextureSampleType_Depth;
    bindingLayoutEntries[8].texture.viewDimension = WGPUTextureViewDimension::WGPUTextureViewDimension_2D;

    bindingLayoutEntries[9].binding = 9u;
    bindingLayoutEntries[9].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[9].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[9].buffer.minBindingSize = sizeof(PointLight);

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
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Vertex;
    bindingLayoutEntries[1].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_ReadOnlyStorage;
    bindingLayoutEntries[1].buffer.minBindingSize = sizeof(glm::vec4) * 400000u;

    bindingLayoutEntries[2].binding = 2u;
    bindingLayoutEntries[2].visibility = WGPUShaderStage_Vertex;
    bindingLayoutEntries[2].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_ReadOnlyStorage;
    bindingLayoutEntries[2].buffer.minBindingSize = sizeof(glm::vec4) * 400000u;

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

std::vector<WGPUBindGroupLayout> Isometric::OnBindGroupLayoutsBlur() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(2);
    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[0].sampler.type = WGPUSamplerBindingType::WGPUSamplerBindingType_Filtering;

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[1].texture.sampleType = WGPUTextureSampleType::WGPUTextureSampleType_Float;
    bindingLayoutEntries[1].texture.viewDimension = WGPUTextureViewDimension::WGPUTextureViewDimension_2D;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

std::vector<WGPUBindGroupLayout> Isometric::OnBindGroupLayoutsEmission() {
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

std::vector<WGPUBindGroup> Isometric::OnBindGroupsPlayerEmission() {
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
    bindGroupEntries[2].sampler = wgpContext.getSampler(SS_LINEAR_CLAMP);

    bindGroupEntries[3].binding = 3u;
    bindGroupEntries[3].textureView = m_wgpPlayerE.getTextureView();


    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_PLAYER_EMISSION"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    bindGroups[0] = wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);

    return bindGroups;
}

std::vector<WGPUBindGroup> Isometric::OnBindGroupsGunEmission() {
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
    bindGroupEntries[2].sampler = wgpContext.getSampler(SS_LINEAR_CLAMP);

    bindGroupEntries[3].binding = 3u;
    bindGroupEntries[3].textureView = m_wgpGunE.getTextureView();


    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_PLAYER_EMISSION"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    bindGroups[0] = wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);

    return bindGroups;
}

std::vector<WGPUBindGroupLayout> Isometric::OnBindGroupLayoutsFloorEmission() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(1);
    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Vertex;
    bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[0].buffer.minBindingSize = sizeof(Uniforms);

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

std::vector<WGPUBindGroupLayout> Isometric::OnBindGroupLayoutsComposite() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(4);
    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[0].sampler.type = WGPUSamplerBindingType::WGPUSamplerBindingType_Filtering;

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[1].texture.sampleType = WGPUTextureSampleType::WGPUTextureSampleType_UnfilterableFloat;
    bindingLayoutEntries[1].texture.viewDimension = WGPUTextureViewDimension::WGPUTextureViewDimension_2D;
    bindingLayoutEntries[1].texture.multisampled = true;

    bindingLayoutEntries[2].binding = 2u;
    bindingLayoutEntries[2].visibility = WGPUShaderStage_Fragment;
    bindingLayoutEntries[2].texture.sampleType = WGPUTextureSampleType::WGPUTextureSampleType_Float;
    bindingLayoutEntries[2].texture.viewDimension = WGPUTextureViewDimension::WGPUTextureViewDimension_2D;

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

std::vector<WGPUBindGroup> Isometric::OnBindGroupsPlayer() {
    std::vector<WGPUBindGroup> bindGroups(1);

    std::vector<WGPUBindGroupEntry> bindGroupEntries(9);
    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].buffer = m_uniformBuffer.getBuffer();
    bindGroupEntries[0].offset = 0u;
    bindGroupEntries[0].size = sizeof(Uniforms);

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].buffer = m_skinBuffer.getBuffer();
    bindGroupEntries[1].offset = 0u;
    bindGroupEntries[1].size = wgpuBufferGetSize(m_skinBuffer.getBuffer());

    bindGroupEntries[2].binding = 2u;
    bindGroupEntries[2].sampler = wgpContext.getSampler(SS_NEAREST_CLAMP);

    bindGroupEntries[3].binding = 3u;
    bindGroupEntries[3].textureView = m_wgpPlayerD.getTextureView();

    bindGroupEntries[4].binding = 4u;
    bindGroupEntries[4].textureView = m_wgpPlayerN.getTextureView();

    bindGroupEntries[5].binding = 5u;
    bindGroupEntries[5].textureView = m_wgpPlayerS.getTextureView();

    bindGroupEntries[6].binding = 6u;
    bindGroupEntries[6].sampler = wgpContext.getSampler(SS_0);

    bindGroupEntries[7].binding = 7u;
    bindGroupEntries[7].textureView = m_wgpTextureShadow.getTextureView();

    bindGroupEntries[8].binding = 8u;
    bindGroupEntries[8].buffer = m_pointLightBuffer.getBuffer();
    bindGroupEntries[8].offset = 0u;
    bindGroupEntries[8].size = wgpuBufferGetSize(m_pointLightBuffer.getBuffer());

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_ANIMATION"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    bindGroups[0] = wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);

    return bindGroups;
}

std::vector<WGPUBindGroup> Isometric::OnBindGroupsGun() {
    std::vector<WGPUBindGroup> bindGroups(1);

    std::vector<WGPUBindGroupEntry> bindGroupEntries(9);
    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].buffer = m_uniformBuffer.getBuffer();
    bindGroupEntries[0].offset = 0u;
    bindGroupEntries[0].size = sizeof(Uniforms);

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].buffer = m_skinBuffer.getBuffer();
    bindGroupEntries[1].offset = 0u;
    bindGroupEntries[1].size = wgpuBufferGetSize(m_skinBuffer.getBuffer());

    bindGroupEntries[2].binding = 2u;
    bindGroupEntries[2].sampler = wgpContext.getSampler(SS_LINEAR_CLAMP);

    bindGroupEntries[3].binding = 3u;
    bindGroupEntries[3].textureView = m_wgpGunD.getTextureView();

    bindGroupEntries[4].binding = 4u;
    bindGroupEntries[4].textureView = m_wgpGunN.getTextureView();

    bindGroupEntries[5].binding = 5u;
    bindGroupEntries[5].textureView = m_wgpGunS.getTextureView();

    bindGroupEntries[6].binding = 6u;
    bindGroupEntries[6].sampler = wgpContext.getSampler(SS_0);

    bindGroupEntries[7].binding = 7u;
    bindGroupEntries[7].textureView = m_wgpTextureShadow.getTextureView();

    bindGroupEntries[8].binding = 8u;
    bindGroupEntries[8].buffer = m_pointLightBuffer.getBuffer();
    bindGroupEntries[8].offset = 0u;
    bindGroupEntries[8].size = wgpuBufferGetSize(m_pointLightBuffer.getBuffer());

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_ANIMATION"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    bindGroups[0] = wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);

    return bindGroups;
}

std::vector<WGPUBindGroup> Isometric::OnBindGroupsFloor() {
    std::vector<WGPUBindGroup> bindGroups(1);

    std::vector<WGPUBindGroupEntry> bindGroupEntries(9);
    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].buffer = m_uniformBuffer.getBuffer();
    bindGroupEntries[0].offset = 0u;
    bindGroupEntries[0].size = sizeof(Uniforms);

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].sampler = wgpContext.getSampler(SS_LINEAR_REPEAT);

    bindGroupEntries[2].binding = 2u;
    bindGroupEntries[2].textureView = m_wgpFloorD.getTextureView();

    bindGroupEntries[3].binding = 3u;
    bindGroupEntries[3].textureView = m_wgpFloorN.getTextureView();

    bindGroupEntries[4].binding = 4u;
    bindGroupEntries[4].textureView = m_wgpFloorS.getTextureView();

    bindGroupEntries[5].binding = 5u;
    bindGroupEntries[5].sampler = wgpContext.getSampler(SS_0);

    bindGroupEntries[6].binding = 6u;
    bindGroupEntries[6].textureView = m_wgpTextureShadow.getTextureView();

    bindGroupEntries[7].binding = 7u;
    bindGroupEntries[7].buffer = m_directionalLightBuffer.getBuffer();
    bindGroupEntries[7].offset = 0u;
    bindGroupEntries[7].size = wgpuBufferGetSize(m_directionalLightBuffer.getBuffer());

    bindGroupEntries[8].binding = 8u;
    bindGroupEntries[8].buffer = m_pointLightBuffer.getBuffer();
    bindGroupEntries[8].offset = 0u;
    bindGroupEntries[8].size = wgpuBufferGetSize(m_pointLightBuffer.getBuffer());

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

std::vector<WGPUBindGroup> Isometric::OnBindGroupsFloorEmission() {
    std::vector<WGPUBindGroup> bindGroups(1);

    std::vector<WGPUBindGroupEntry> bindGroupEntries(1);
    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].buffer = m_uniformBuffer.getBuffer();
    bindGroupEntries[0].offset = 0u;
    bindGroupEntries[0].size = sizeof(Uniforms);

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_FLOOR_EMISSION"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    bindGroups[0] = wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);

    return bindGroups;
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

WGPUBindGroup Isometric::createBindGroupWiggly() {
    std::vector<WGPUBindGroupEntry> bindGroupEntries(10);
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
    bindGroupEntries[5].textureView = m_wgpEnemyN.getTextureView();

    bindGroupEntries[6].binding = 6u;
    bindGroupEntries[6].textureView = m_wgpEnemyS.getTextureView();

    bindGroupEntries[7].binding = 7u;
    bindGroupEntries[7].sampler = wgpContext.getSampler(SS_0);

    bindGroupEntries[8].binding = 8u;
    bindGroupEntries[8].textureView = m_wgpTextureShadow.getTextureView();

    bindGroupEntries[9].binding = 9u;
    bindGroupEntries[9].buffer = m_pointLightBuffer.getBuffer();
    bindGroupEntries[9].offset = 0u;
    bindGroupEntries[9].size = wgpuBufferGetSize(m_pointLightBuffer.getBuffer());

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_WIGGLY"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    return wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
}

WGPUBindGroup Isometric::createBindGroupComposite() {
    std::vector<WGPUBindGroupEntry> bindGroupEntries(4);
    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].sampler = wgpContext.getSampler(SS_LINEAR_CLAMP);

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].textureView = m_wgpSceneTarget.getTextureView();

    bindGroupEntries[2].binding = 2u;
    bindGroupEntries[2].textureView = m_wgpBlurFinalTarget.getTextureView();

    bindGroupEntries[3].binding = 3u;
    bindGroupEntries[3].textureView = m_wgpEmissionTarget.getTextureView();

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_COMPOSITE"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    return wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
}

WGPUBindGroup Isometric::createBindGroupBlurH() {
    std::vector<WGPUBindGroupEntry> bindGroupEntries(2);
    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].sampler = wgpContext.getSampler(SS_LINEAR_CLAMP);

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].textureView = m_wgpEmissionTarget.getTextureView();

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_BLUR_HORIZONTAL"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    return wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
}

WGPUBindGroup Isometric::createBindGroupBlurV() {
    std::vector<WGPUBindGroupEntry> bindGroupEntries(2);
    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].sampler = wgpContext.getSampler(SS_LINEAR_CLAMP);

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].textureView = m_wgpBlurTempTarget.getTextureView();

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_BLUR_VERTICAL"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();

    return wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);
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