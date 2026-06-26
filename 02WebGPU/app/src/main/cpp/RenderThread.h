#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <android/native_window.h>

class RenderThread {

public:
    RenderThread() : m_renderThread(), m_running(false), m_window(nullptr) {}
    ~RenderThread() { stop(); }

    void start();
    void stop();
    void setWindow(ANativeWindow* window);

private:
    void threadLoop();

    std::thread m_renderThread;
    std::atomic<bool> m_running;

    std::mutex m_Mutex;
    std::condition_variable m_cv;
    ANativeWindow* m_window;
};