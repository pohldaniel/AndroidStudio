#include <glm/gtc/type_ptr.hpp>

#include <WebGPU/WgpContext.h>
#include <WebGPU/WgpRenderer.h>

#include <Nuklear/NkContext.h>
#include <Nuklear/NkStyle.h>

#include <States/VideoDecode.h>
#include <States/Isometric.h>

#include <Physics/Physics.h>

#include "Globals.h"
#include "InputTouch.h"
#include "Cubes.h"

Cubes::Cubes(StateMachine& machine) : State(machine, States::CUBES){
    nkInit(static_cast<float>(wgpWidth), static_cast<float>(wgpHeight));
    nkInitFont("fonts/upheavtt.ttf");
    Physics::DebugDrawer.init();

    m_camera.perspective(glm::radians(45.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 1000.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), 0.0f, static_cast<float>(wgpHeight), -1.0f, 1.0f);
    m_camera.lookAt(glm::vec3(10.0f, 4.3f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_camera.setMovingSpeed(10.0f);
    m_camera.setRotationSpeed(0.1f);

    m_trackball.reshape(wgpWidth, wgpHeight);

    m_uniformBuffer.createBuffer(sizeof(Uniforms), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform);

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

    wgpContext.setClearColor({ 0.7f, 0.7f, 0.7f, 1.0f });
    wgpContext.OnDraw = std::bind(&Cubes::OnDraw, this, std::placeholders::_1, std::placeholders::_2);
    nkContext.OnFillBuffer = std::bind(&Cubes::OnFillBuffer, this, std::placeholders::_1);

    m_scene = new SceneNode();
    m_scene->setOnChildAdded([this](Node* newNode) {
        if (auto* sceneNode = dynamic_cast<SceneNode*>(newNode)) {
            m_children.push_back(sceneNode);
        }
    });

    m_scene->setOnChildRemoved([this](Node* removedNode) {
        auto it = std::find(m_children.begin(), m_children.end(), removedNode);
        if (it != m_children.end()) {
            std::iter_swap(it, m_children.end() - 1);
            m_children.pop_back();
        }
    });

    btCollisionObject* body = Physics::AddStaticObject(Physics::BtTransform(btVector3(0.0f, -50.0f, 0.0f)), new btBoxShape(btVector3(50.0f, 50.0f, 50.0f)), Physics::collisiontypes::FLOOR, Physics::collisiontypes::CUBE, nullptr);
    SceneNode* node = m_scene->addChild<SceneNode>();
    node->setScale(250.0f * 2.0f, 250.0f * 2.0f, 250.0f * 2.0f);
    node->setPosition(0.0f, -50.0f, 0.0f);

    btTransform startTransform;
    startTransform.setIdentity();
    for (int k = 0; k < ARRAY_SIZE_Y; k++){
        for (int i = 0; i < ARRAY_SIZE_X; i++){
            for (int j = 0; j < ARRAY_SIZE_Z; j++){
                startTransform.setOrigin(btVector3(btScalar(0.2 * i), btScalar(2 + .2 * k), btScalar(0.2 * j)));
                btRigidBody* body = Physics::AddRigidBody(1.0f, startTransform, new btBoxShape(btVector3(0.1f, 0.1f, 0.1f)), Physics::collisiontypes::CUBE, Physics::collisiontypes::CUBE | Physics::collisiontypes::FLOOR);
                m_scene->addChild<CollisionNode>(body);
            }
        }
    }

    m_storageBuffer.createBuffer(5000u * sizeof(GPUInstanceData), WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    wgpContext.addSahderModule("STORAGE", "shader/physics.wgsl");
    wgpContext.createRenderPipeline("STORAGE", "RP_STORAGE", VL_P, std::bind(&Cubes::OnBindGroupLayouts, this));

    m_cube.buildCube({ -0.1f, -0.1f, -0.1f }, { 0.2f, 0.2f, 0.2f }, 1u, 1u, false, false);
    m_wgpCube.create(m_cube);
    m_wgpCube.setBindGroups("BG", std::bind(&Cubes::OnBindGroups, this));
}

Cubes::~Cubes() {
    delete m_scene;
    m_uniformBuffer.markForDelete();
    m_storageBuffer.markForDelete();
}

void Cubes::fixedUpdate() {
    Globals::physics->stepSimulation(FIXED_STEP);
}

void Cubes::update() {
	nkUpdateInput(0, 0, false, false, 0.0f);
    if(touchStates[0].touchPressed){
        shootCube(touchStates[0].touchX, touchStates[0].touchY);
    }

    m_uniforms.projection = m_camera.getPerspectiveMatrix();
    m_uniforms.view = m_camera.getViewMatrix();
    m_uniforms.env = m_camera.getRotationMatrix();
    m_uniforms.model = glm::mat4(1.0f);
    m_uniforms.normal = Camera::GetNormalMatrix(m_camera.getViewMatrix() * m_uniforms.model);
    m_uniforms.camPosition = m_camera.getPosition();
    m_uniforms.lightVP = glm::mat4(1.0f);
    m_uniforms.shadow = Camera::BIAS * m_uniforms.lightVP;
    wgpuQueueWriteBuffer(wgpContext.queue, m_uniformBuffer.getBuffer(), 0, &m_uniforms, sizeof(Uniforms));

    if (m_debugPhysic) {
        glm::mat4 viewProjection = m_camera.getPerspectiveMatrix() * m_camera.getViewMatrix();
        float* targetArray = Physics::DebugDrawer.getViewProjection();
        std::copy(glm::value_ptr(viewProjection), glm::value_ptr(viewProjection) + 16, targetArray);
    }

    m_cpuInstanceBuffer.clear();
    m_cpuInstanceBuffer.reserve(m_scene->getChildren().size());

    int counter = 0;
    bool first = true;
    for (const auto& child : m_children) {
        GPUInstanceData data;
        data.modelMatrix = child->getWorldTransformation();
        data.color = first ? glm::vec4(161.0f / 256.0f, 155.0f / 256.0f, 114.0f / 256.0f, 1.0f) : colors[counter];
        m_cpuInstanceBuffer.push_back(data);
        counter = (counter + 1) % 4;
        first = false;
    }

    wgpuQueueWriteBuffer(wgpContext.queue, m_storageBuffer.getBuffer(), 0u, m_cpuInstanceBuffer.data(), m_cpuInstanceBuffer.size() * sizeof(GPUInstanceData));
}

void Cubes::render() {
    wgpDraw();
}

void Cubes::OnDraw(const WGPUCommandEncoder& commandEncoder, const WGPURenderPassDescriptor& renderPassDescriptor) {
    {
        WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);
        wgpuRenderPassEncoderSetViewport(renderPassEncoder, 0.0f, 0.0f, static_cast<float>(wgpWidth), static_cast<float>(wgpHeight), 0.0f, 1.0f);
        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, wgpContext.renderPipelines.at("RP_STORAGE"));

        m_wgpCube.draw(renderPassEncoder, m_cpuInstanceBuffer.size());

        wgpuRenderPassEncoderEnd(renderPassEncoder);
        wgpuRenderPassEncoderRelease(renderPassEncoder);
    }

    if(m_debugPhysic)
    {
        Physics::GetDynamicsWorld()->debugDrawWorld();
        Physics::DebugDrawer.OnDraw(commandEncoder, renderPassDescriptor);
    }
}

void Cubes::OnFillBuffer(nk_context& nkCntxt) {

}

void Cubes::resize(int deltaW, int deltaH) {
    nkResize(static_cast<float>(wgpWidth), static_cast<float>(wgpHeight));
    m_camera.perspective(glm::radians(45.0f), static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight), 0.1f, 1000.0f);
    m_camera.orthographic(0.0f, static_cast<float>(wgpWidth), 0.0f, static_cast<float>(wgpHeight), -1.0f, 1.0f);
    m_trackball.reshape(wgpWidth, wgpHeight);
}

void Cubes::OnButton(const Event::MouseButtonEvent& event) {
    wgpCleanState();
    nkShutDown();
    Physics::DebugDrawer.shutDown();
    m_isRunning = false;

    if(event.button == Event::MouseButtonEvent::BUTTON_LEFT){
        m_machine.addStateAtBottom(new VideoDecode(m_machine));
    }

    if(event.button == Event::MouseButtonEvent::BUTTON_RIGHT){
        m_machine.addStateAtBottom(new Isometric(m_machine));
    }
}

std::vector<WGPUBindGroupLayout> Cubes::OnBindGroupLayouts() {
    std::vector<WGPUBindGroupLayout> bindingLayouts(1);

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(2);
    bindingLayoutEntries[0].binding = 0u;
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Vertex;
    bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[0].buffer.minBindingSize = sizeof(Uniforms);

    bindingLayoutEntries[1].binding = 1u;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Vertex;
    bindingLayoutEntries[1].buffer.type = WGPUBufferBindingType::WGPUBufferBindingType_ReadOnlyStorage;
    bindingLayoutEntries[1].buffer.minBindingSize = 126 * sizeof(GPUInstanceData);

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();

    bindingLayouts[0] = wgpuDeviceCreateBindGroupLayout(wgpContext.device, &bindGroupLayoutDescriptor);

    return bindingLayouts;
}

std::vector<WGPUBindGroup> Cubes::OnBindGroups() {
    std::vector<WGPUBindGroup> bindGroups(1);

    std::vector<WGPUBindGroupEntry> bindGroupEntries(2);

    bindGroupEntries[0].binding = 0u;
    bindGroupEntries[0].buffer = m_uniformBuffer.getBuffer();
    bindGroupEntries[0].offset = 0u;
    bindGroupEntries[0].size = wgpuBufferGetSize(m_uniformBuffer.getBuffer());

    bindGroupEntries[1].binding = 1u;
    bindGroupEntries[1].buffer = m_storageBuffer.getBuffer();
    bindGroupEntries[1].offset = 0u;
    bindGroupEntries[1].size = wgpuBufferGetSize(m_storageBuffer.getBuffer());

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(wgpContext.renderPipelines.at("RP_STORAGE"), 0u);
    bindGroupDesc.entryCount = (uint32_t)bindGroupEntries.size();
    bindGroupDesc.entries = bindGroupEntries.data();
    bindGroups[0] = wgpuDeviceCreateBindGroup(wgpContext.device, &bindGroupDesc);

    return bindGroups;
}

void Cubes::shootCube(float posX, float posY) {
    float mouseXndc = (2.0f * posX) / static_cast<float>(wgpWidth) - 1.0f;
    float mouseYndc = 1.0f - (2.0f * posY) / static_cast<float>(wgpHeight);
    float tanfov = m_camera.getTanFov();
    float aspect = (static_cast<float>(wgpWidth) / static_cast<float>(wgpHeight));

    glm::vec3 rayStartWorld = m_camera.getPosition() + (m_camera.getCamX() * mouseXndc * tanfov * aspect + m_camera.getCamY() * mouseYndc * tanfov + m_camera.getViewDirection()) * m_camera.getNear();
    glm::vec3 rayEndWorld = m_camera.getPosition() + (m_camera.getCamX() * mouseXndc * tanfov * aspect + m_camera.getCamY() * mouseYndc * tanfov + m_camera.getViewDirection()) * m_camera.getFar();

    glm::vec3 shootDirection = glm::normalize(rayEndWorld - rayStartWorld);
    glm::vec3 spawnPos = m_camera.getPosition() + shootDirection * 1.5f;
    float shootForce = 40.0f;
    glm::vec3 velocity = shootDirection * shootForce;

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(Physics::VectorFrom(spawnPos));
    btRigidBody* body = Physics::AddRigidBody(1.0f, transform, new btBoxShape(btVector3(0.1f, 0.1f, 0.1f)), Physics::collisiontypes::CUBE, Physics::collisiontypes::CUBE | Physics::collisiontypes::FLOOR);

    auto* cubeNode = m_scene->addChild<CollisionNode>(body);
    body->setLinearVelocity(Physics::VectorFrom(velocity));
}