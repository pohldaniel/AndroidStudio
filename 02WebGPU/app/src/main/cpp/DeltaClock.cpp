#include "DeltaClock.h"

float Clock() {
    static timespec _base;
    static bool firstCall = true;

    if (firstCall) {
        clock_gettime(CLOCK_MONOTONIC, &_base);
        firstCall = false;
    }

    timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    float secDiff = (float)(t.tv_sec - _base.tv_sec);
    float msecDiff = (float)((t.tv_nsec - _base.tv_nsec) / 1000000);
    return secDiff + 0.001f * msecDiff;
}