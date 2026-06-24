#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <android/native_window.h>

class WgpRenderer {
public:
    WgpRenderer() : m_RenderThread(), m_Running(false), m_Window(nullptr) {}
    ~WgpRenderer() { stop(); }

    void start();
    void stop();
    void setWindow(ANativeWindow* window);

private:
    void threadLoop();

    std::thread m_RenderThread;
    std::atomic<bool> m_Running;

    std::mutex m_Mutex;
    std::condition_variable m_CV;
    ANativeWindow* m_Window; // Zugriffsschutz via Mutex erforderlich
};