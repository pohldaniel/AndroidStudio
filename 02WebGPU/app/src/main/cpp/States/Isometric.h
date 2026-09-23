#pragma once

#include <webgpu.h>

#include <Physics/Physics.h>

#include <WebGPU/WgpData.h>
#include <WebGPU/WgpTexture.h>
#include <WebGPU/WgpBuffer.h>
#include <WebGPU/WgpModel.h>

#include <States/StateMachine.h>
#include <Nuklear/NkJoystick.h>
#include <Shape/Shape.h>

#include <Sound/SoundEffect.h>

#include <core/animation/AnimatedModel.h>
#include <core/animation/AnimationController.h>
#include <core/scene/SceneNode.h>
#include <core/Camera.h>
#include <core/AssimpModel.h>
#include <core/ObjModel.h>
#include <core/TrackBall.h>
#include <core/Transform.h>

#include "bullet_store.h"
#include "enemy_spawner.h"

struct BulletCollisionCallback : public btCollisionWorld::ContactResultCallback {
    bool m_hasCollided = false;
    btCollisionObject* m_hitTarget = nullptr;
    virtual bool needsCollision(btBroadphaseProxy* proxy) const override {

        auto* targetObj = static_cast<btCollisionObject*>(proxy->m_clientObject);
        if (!targetObj)
            return false;

        if (targetObj->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE)
            return false;

        return (Physics::collisiontypes::ENEMY & proxy->m_collisionFilterGroup) && (Physics::collisiontypes::CHARACTER & proxy->m_collisionFilterMask);
    }

    virtual btScalar addSingleResult(btManifoldPoint& cp,
                                     const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0,
                                     const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) override
    {
        m_hasCollided = true;
        m_hitTarget = const_cast<btCollisionObject*>(colObj1Wrap->getCollisionObject());
        return 0;
    }
};

class CollisionEntity;
class Enemy;
class Player;

class Isometric : public State {
    struct Wiggly {
        glm::vec3 nosePos;
        float time;
    };

    struct SpriteInstance {
        float position[3];
        float age;
        float scale[2];
        float currentFrame;
        uint32_t padding2;
    };

    struct FrameInfo {
        uint32_t colRow[2];
        float frameSize[2];
    };

    struct DirectionalLight {
        float direction[3];
        float padding;
        float color[4];
    };

    struct PointLight {
        float position[3];
        float padding;
        float color[4];
        uint32_t active;
        uint32_t pad1;
        uint32_t pad2;
        uint32_t pad3;
    };

public:

    Isometric(StateMachine& machine);
    ~Isometric();

    void fixedUpdate() override;
    void update() override;
    void render() override;
    void resize(int deltaW, int deltaH) override;

    void OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor);
    void OnPostDraw();
    void OnDrawShadow(const WGPURenderPassEncoder& renderPassEncoder);
    void OnDrawEmission(const WGPURenderPassEncoder& renderPassEncoder);
    void OnDrawScene(const WGPURenderPassEncoder& renderPassEncoder);
    void OnFillBuffer(nk_context& nkCntxt);
    void OnButton(const Event::MouseButtonEvent& event) override;

private:

    std::vector<WGPUBindGroupLayout> OnBindGroupLayouts();
    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsFloor();
    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsWiggly();
    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsBullet();
    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsBillboard();
    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsShadow();
    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsWigglyShadow();
    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsBlur();
    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsEmission();
    std::vector<WGPUBindGroup> OnBindGroupsPlayerEmission();
    std::vector<WGPUBindGroup> OnBindGroupsGunEmission();
    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsFloorEmission();
    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsComposite();

    std::vector<WGPUBindGroup> OnBindGroupsPlayer();
    std::vector<WGPUBindGroup> OnBindGroupsGun();
    std::vector<WGPUBindGroup> OnBindGroupsFloor();
    std::vector<WGPUBindGroup> OnBindGroupsBullet();
    std::vector<WGPUBindGroup> OnBindGroupsShadow();
    std::vector<WGPUBindGroup> OnBindGroupsFloorEmission();

    WGPUBindGroup createBindGroupBillboard();
    WGPUBindGroup createBindGroupMuzzle();
    WGPUBindGroup createBindGroupWiggly();
    WGPUBindGroup createBindGroupComposite();
    WGPUBindGroup createBindGroupBlurH();
    WGPUBindGroup createBindGroupBlurV();

    CollisionEntity* createNewBulletToPool();

    void spawnBillboard(const glm::vec3& position);
    void resetMuzzle();
    void updateBillboards(float dt);
    void updateMuzzle(float dt, float x, float y, float z);

    bool m_isDeath = false;
    bool m_wantResize = false;

    Camera m_camera;
    Uniforms m_uniforms;
    TrackBall m_trackball;
    JoystickResult m_joystickResult;
    RotationButtonResult m_rotationButtonResult;
    Wiggly m_wiggly;
    BulletStore m_bulletStore;
    SoundEffect m_fire, m_ding;
    SceneNode* m_scene;

    AssimpModel m_enemy;
    AnimatedModel m_player;
    Shape m_floor, m_bullet;
    Animation m_full;
    WgpBuffer m_uniformBuffer, m_infoBufferBillboard, m_infoBufferMuzzle, m_storageBuffer, m_wigglyBuffer, m_skinBuffer, m_rotationBuffer, m_offsetBuffer, m_spriteBuffer, m_muzzleBuffer;
    WgpBuffer m_pointLightBuffer, m_directionalLightBuffer;
    WgpModel m_wgpPlayer, m_wgpFloor, m_wgpEnemy, m_wgpBullet;

    WgpTexture m_wgpBulletTexture, m_sprite, m_muzzle, m_wgpTextureShadow;
    WGPUBindGroup m_bindGroupBillboard, m_bindGroupMuzzle, m_bindGroupComposite, m_bindGroupBlurH, m_bindGroupBlurV;

    WgpTexture m_wgpFloorD, m_wgpFloorN, m_wgpFloorS, m_wgpEnemyD, m_wgpEnemyN, m_wgpEnemyS, m_wgpPlayerD, m_wgpPlayerN, m_wgpPlayerS, m_wgpPlayerE, m_wgpGunD, m_wgpGunN, m_wgpGunS, m_wgpGunE;

    WgpTexture m_wgpEmissionTarget, m_wgpEmissionDepth;
    WgpTexture m_wgpSceneTarget, m_wgpSceneDepth;
    WgpTexture m_wgpBlurTempTarget;
    WgpTexture m_wgpBlurFinalTarget;

    float prev_idleWeight = 0.0f;
    float prev_rightWeight = 0.0f;
    float prev_forwardWeight = 0.0f;
    float prev_backWeight = 0.0f;
    float prev_leftWeight = 0.0f;
    const float animTransitionTime = 0.2f;
    float aimTheta = 0.0f;
    float lastFireTime = 0.0f;
    int m_spreadAmount = 20;

    EnemySpawner m_enemySpawner;
    std::vector<CollisionEntity*> m_entities;
    Player* m_playerEntity;
    std::vector<Enemy*> m_enemies;
    std::vector<glm::mat4> m_cpuInstanceBuffer;
    std::vector<SpriteInstance> m_activeBillboards;
    SpriteInstance m_muzzleInstance;
    glm::mat4 m_lightProjection, m_lightView;
    glm::vec3 m_lightDir;
    std::vector<float> m_muzzleFlashSpritesAge;

    static WGPUBindGroup CreateBindGroupShadow(const WgpBuffer& uniformBuffer, const WgpBuffer& wigglyBuffer, const WgpBuffer& storageBuffer);
};