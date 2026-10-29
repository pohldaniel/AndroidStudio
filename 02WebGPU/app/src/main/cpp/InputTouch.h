#pragma once
#include <vector>
#include <mutex>

#define MAX_TOUCH_POINTERS 5
struct TouchState{
    float touchX;
    float touchY;
    bool touchActive;
    bool touchPressed;
};
extern TouchState touchStates[MAX_TOUCH_POINTERS];

struct TouchEvent {
    int pointerId;
    float x;
    float y;
    int action;
};
extern std::vector<TouchEvent> TouchEventQueue;
extern std::mutex TouchMutex;