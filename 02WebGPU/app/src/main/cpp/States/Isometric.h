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

        return (Physics::collisiontypes::ENEMY & proxy->m_collisionFilterGroup) && (Physics::collisiontypes::SPHERE & proxy->m_collisionFilterMask);
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

struct BulletCollisionPlayerCallback : public btCollisionWorld::ContactResultCallback {
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

public:

	Isometric(StateMachine& machine);
    ~Isometric();

    void fixedUpdate() override;
    void update() override;
    void render() override;
    void resize(int deltaW, int deltaH) override;

    void OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor);
	void OnFillBuffer(nk_context& nkCntxt);
    void OnButton(const Event::MouseButtonEvent& event) override;

private:

    std::vector<WGPUBindGroupLayout> OnBindGroupLayouts();
    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsFloor();
    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsWiggly();
    std::vector<WGPUBindGroupLayout> OnBindGroupLayoutsBullet();

    std::vector<WGPUBindGroup> OnBindGroups();
    std::vector<WGPUBindGroup> OnBindGroupsFloor();
    std::vector<WGPUBindGroup> OnBindGroupsBullet();

    void renderUi(const WGPURenderPassEncoder& renderPassEncoder);
    float getLookAtYRotation(const glm::vec3& objectPos, const glm::vec3& targetPos);
    CollisionEntity* createNewBulletToPool();

    bool m_initUi = true;
    bool m_drawUi = false;
    bool m_isDeath = false;
    bool m_debugCollision = false;

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
    WgpBuffer m_uniformBuffer, m_storageBuffer, m_wigglyBuffer, m_skinBuffer, m_rotationBuffer, m_offsetBuffer;
    WgpModel m_wgpPlayer, m_wgpFloor, m_wgpEnemy, m_wgpBullet;
    WgpTexture m_wgpFloorD, m_wgpEnemyD, m_wgpBulletTexture;

    float prev_idleWeight = 0.0f;
    float prev_rightWeight = 0.0f;
    float prev_forwardWeight = 0.0f;
    float prev_backWeight = 0.0f;
    float prev_leftWeight = 0.0f;
    const float animTransitionTime = 0.2f;
    float deathTime = -1.0f;
    float lastFireTime = 0.0f;
    size_t m_targetPoolSize;

    EnemySpawner m_enemySpawner;
    std::vector<CollisionEntity*> m_entities;
    Player* m_playerEnitity;
    std::vector<Enemy*> m_enemies;
    std::vector<glm::mat4> m_cpuInstanceBuffer;

    static WGPUBindGroup CreateBindGroup(const WgpBuffer& uniformBuffer, const WgpBuffer& wigglyBuffer, const WgpTexture& texture, const WgpBuffer& storageBuffer);
};