#include "WgpContext.h"
#include "WgpRenderer.h"

void WgpRenderer::start() {
    if (m_Running) return;

    m_Running = true;
    m_RenderThread = std::thread(&WgpRenderer::threadLoop, this);

}

void WgpRenderer::stop() {
    if (!m_Running) return;

    m_Running = false;
    m_CV.notify_one();

    if (m_RenderThread.joinable()) {
        m_RenderThread.join();
    }
}

void WgpRenderer::setWindow(ANativeWindow* window) {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_Window = window;
    lock.unlock();

    m_CV.notify_one();
}

void WgpRenderer::threadLoop() {
    while (m_Running) {
        std::unique_lock<std::mutex> lock(m_Mutex);

        while (m_Window == nullptr && m_Running) {
            m_CV.wait(lock);
        }

        if (!m_Running) break;

        wgpDraw();
    }
}