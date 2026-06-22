#include <ctime>

template<typename T> T Clamp(T v, T min, T max) {
    return (v < min) ? min : (v > max) ? max : v;
}

float Clock();

class DeltaClock {
    private:
        float mLastTick;
        float mMaxDelta;
        bool mHasMax;
    public:

        DeltaClock() {
            mLastTick = Clock();
            mHasMax = false;
        }
        DeltaClock(float maxDelta) {
            mLastTick = Clock();
            mMaxDelta = maxDelta;
            mHasMax = true;
        }
        float ReadDelta() {
            float d = Clamp(Clock() - mLastTick, 0.0f, mMaxDelta);
            mLastTick = Clock();
            return d;
        }
        void SetMaxDelta(float m) {
            mMaxDelta = m;
        }
        void Reset() {
            mLastTick = Clock();
        }
};