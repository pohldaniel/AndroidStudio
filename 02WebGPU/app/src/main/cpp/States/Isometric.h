#pragma once

#include <webgpu.h>

#include <WebGPU/WgpData.h>
#include <WebGPU/WgpTexture.h>
#include <WebGPU/WgpBuffer.h>
#include <WebGPU/WgpModel.h>

#include <States/StateMachine.h>
#include <Nuklear/NkJoystick.h>
#include <Shape/Shape.h>

#include <core/animation/AnimatedModel.h>
#include <core/animation/AnimationController.h>
#include <core/Camera.h>
#include <core/AssimpModel.h>
#include <core/ObjModel.h>
#include <core/TrackBall.h>
#include <core/Transform.h>

#include "bullet_store.h"

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

    bool m_initUi = true;
    bool m_drawUi = false;
    bool m_isDeath = false;

    Camera m_camera;
    Uniforms m_uniforms;
    TrackBall m_trackball;
    JoystickResult m_joystickResult;
    RotationButtonResult m_rotationButtonResult;
    Wiggly m_wiggly;
    BulletStore m_bulletStore;

    AssimpModel m_enemy;
    AnimatedModel m_player;
    Shape m_floor, m_bullet;
    Animation m_full;
    WgpBuffer m_uniformBuffer, m_instanceBuffer, m_wigglyBuffer, m_skinBuffer, m_rotationBuffer, m_offsetBuffer;
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

    static WGPUBindGroup CreateBindGroup(const WgpBuffer& uniformBuffer, const WgpBuffer& wigglyBuffer, const WgpTexture& texture);
};