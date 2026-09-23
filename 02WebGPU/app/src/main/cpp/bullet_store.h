#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "ThreadPool.h"

class Enemy;
class BulletStore {
public:
    struct BulletGroup {
        int startIndex;
        int groupSize;
        float TTL;

        BulletGroup(int _startIndex, int _groupSize, float lifetime)
        {
            TTL = lifetime;
            startIndex = _startIndex;
            groupSize = _groupSize;
        }
    };

    BulletStore(ThreadPool* const _threadPool)
            : threadPool(_threadPool) {}

    void createBullets(const glm::vec3& position, const glm::quat& midOri, const int spreadAmount);
    void updateBullets(float deltaTimeSeconds, std::vector<Enemy*>& enemies);

    ThreadPool* const threadPool;
    std::vector<BulletGroup> bulletGroups;

    std::vector<glm::vec4> m_offsets;
    std::vector<glm::quat> m_rots;
    std::vector<glm::vec3> allBulletDirs;
};