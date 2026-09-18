#pragma once
#include <vector>
#include <animation/AnimatedModel.h>
#include <entities/Enemy.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class EnemySpawner {

public:

	EnemySpawner(float monsterY, const AnimatedModel& player);

	void update(const glm::vec3& pos, float dt);
	SceneNode* scene;
	size_t& count();

private:

	void spawnEnemy(const glm::vec3& pos);
	float getLookAtYRotation(const glm::vec3& objectPos, const glm::vec3& targetPos);
	
	float m_countdown;
	const float m_monsterY;
	size_t m_count;

	const AnimatedModel& player;
};