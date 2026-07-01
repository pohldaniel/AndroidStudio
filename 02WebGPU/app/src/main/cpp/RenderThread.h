#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <android/native_window.h>

class DeltaClock;
class StateMachine;

class RenderThread {

public:

    RenderThread(const DeltaClock& deltaClock, StateMachine& stateMachine) :
    m_renderThread(),
    m_mutex(),
    m_running(false),
    m_wantPause(false),
    m_pause(false),
    m_blockUiThreadCv(),
    m_readyToRenderCv(),
    m_window(nullptr),
    deltaClock(deltaClock),
    stateMachine(stateMachine){

    }

    ~RenderThread() {
        stop();
    }

    void start();
    void stop();
    void setWindow(ANativeWindow* window);
    void pause();
    void resume();

private:

    void threadLoop();

    std::thread m_renderThread;
    std::mutex m_mutex;
    std::atomic<bool> m_running;
    std::atomic<bool> m_wantPause;
    std::atomic<bool> m_pause;

    std::condition_variable m_blockUiThreadCv;
    std::condition_variable m_readyToRenderCv;

    ANativeWindow* m_window;
    const DeltaClock& deltaClock;
    StateMachine& stateMachine;
};