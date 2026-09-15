#pragma once

#define MAX_TOUCH_POINTERS 5
struct TouchState{
    float touchX;
    float touchY;
    bool touchActive;
    bool touchPressed;
};
extern TouchState touchStates[MAX_TOUCH_POINTERS];

