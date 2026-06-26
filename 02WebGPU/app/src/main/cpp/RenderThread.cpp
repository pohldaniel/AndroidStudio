#include "WebGPU/WgpContext.h"
#include "RenderThread.h"

void RenderThread::start() {
    if (m_running) return;

    m_running = true;
    m_renderThread = std::thread(&RenderThread::threadLoop, this);

}

void RenderThread::stop() {
    if (!m_running) return;

    m_running = false;
    m_cv.notify_one();

    if (m_renderThread.joinable()) {
        m_renderThread.join();
    }
}

void RenderThread::setWindow(ANativeWindow* window) {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_window = window;
    lock.unlock();

    m_cv.notify_one();
}

void RenderThread::threadLoop() {
    while (m_running) {
        std::unique_lock<std::mutex> lock(m_Mutex);

        while (m_window == nullptr && m_running) {
            m_cv.wait(lock);
        }

        if (!m_running) break;

        wgpDraw();
    }
}