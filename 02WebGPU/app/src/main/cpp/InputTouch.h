#pragma once

#define MAX_TOUCH_POINTERS 5
struct TouchState{
    float touchX;
    float touchY;
    bool touchActive;
};

extern TouchState touchStates[MAX_TOUCH_POINTERS];