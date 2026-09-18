#include <cstdlib>

#include "enemy_spawner.h"

namespace {
    const float enemySpawnInterval = 1.0f;
    const int spawnsPerInterval = 1;
    const float spawnRadius = 10.0f;
}

EnemySpawner::EnemySpawner(float monsterY, const AnimatedModel& player) : m_countdown(spawnsPerInterval), m_monsterY(monsterY), m_count(0), player(player){

}

void EnemySpawner::update(const glm::vec3& pos, float dt) {
    m_countdown -= dt;
    if (m_countdown <= 0.0f) {
        for (int i = 0; i < spawnsPerInterval; ++i) {
            spawnEnemy(pos);
        }
        m_countdown += enemySpawnInterval;
    }
}

void EnemySpawner::spawnEnemy(const glm::vec3& pos) {
    if (m_count > 20u)
        return;

    const float theta = glm::radians((float)(rand() % 360));
    const float x = pos[0] + sin(theta) * spawnRadius;
    const float z = pos[2] + cos(theta) * spawnRadius;

    glm::vec3 spawnPos(x, m_monsterY, z);
    glm::quat rot = glm::quat(glm::vec3(0.0f, getLookAtYRotation(pos, spawnPos), 0.0f));
  
    btCollisionObject* body = Physics::AddKinematicObject(Physics::BtTransform(spawnPos, rot), new btCapsuleShapeZ(0.08f, 0.4f), Physics::collisiontypes::ENEMY, Physics::collisiontypes::SPHERE | Physics::collisiontypes::CHARACTER);
    Enemy* enemy = scene->addChild<Enemy>(body, static_cast<const AnimatedMesh*>(player.getMesh())->getBone(0u).getPosition());
    enemy->setPosition(spawnPos);
    enemy->setOrientation(rot);   
}

float EnemySpawner::getLookAtYRotation(const glm::vec3& objectPos, const glm::vec3& targetPos) {
    float dx = targetPos[0] - objectPos[0];
	float dz = targetPos[2] - objectPos[2];

	if (abs(dx) < 0.01f && abs(dz) < 0.01f)
		return 0.0f;

	return std::atan2(dx, dz);
}

size_t& EnemySpawner::count() {
    return m_count;
}