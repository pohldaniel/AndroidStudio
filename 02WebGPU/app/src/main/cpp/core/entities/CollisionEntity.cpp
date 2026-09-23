#include "CollisionEntity.h"

CollisionEntity::CollisionEntity(btCollisionObject* collisionObject) : CollisionNode(collisionObject){

}

CollisionEntity::~CollisionEntity() {

}

void CollisionEntity::update(float dt) {

}

void CollisionEntity::fixedUpdate(float fdt) {
    if (!m_isActive) return;

    const glm::vec3& pos = getPosition();
    const glm::quat& rot = getOrientation();
    m_collisionObject->setWorldTransform(Physics::BtTransform(pos, rot));
}