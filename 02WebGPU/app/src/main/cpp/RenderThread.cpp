#include "WebGPU/WgpContext.h"
#include "States/StateMachine.h"
#include "DeltaClock.h"
#include "RenderThread.h"

void RenderThread::start() {
    if (m_running)
        return;

    m_running = true;
    m_renderThread = std::thread(&RenderThread::threadLoop, this);

}

void RenderThread::stop() {
    if (!m_running)
        return;

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_running = false;
    }

    // Wake up the thread so it can notice m_running is false and exit the loop
    m_readyToRenderCv.notify_one();

    if (m_renderThread.joinable()) {
        m_renderThread.join();
    }
}

void RenderThread::pause() {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_wantPause = true;

        //Signals the RenderThread to stop rendering the next frame
        m_readyToRenderCv.notify_one();


        //Blocks the Android UI Thread until the RenderThread signal it's done with the current cycle
        //Releases the mutex as well
        m_blockUiThreadCv.wait(lock, [this]() {
            return m_pause.load();
        });
    }
}

void RenderThread::resume() {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_wantPause = false;
        m_pause = false;
    }
    //The Android UI Thread wakes the RenderThread with released mutex
    m_readyToRenderCv.notify_one();
}

void RenderThread::setWindow(ANativeWindow* window) {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_window = window;
        lock.unlock();

        m_readyToRenderCv.notify_one();
    }
}

void RenderThread::threadLoop() {
    while (m_running) {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (m_window == nullptr || m_wantPause) {
            m_pause = true;
            m_blockUiThreadCv.notify_all();
        }

        //thread is waiting until condition from .wait() is fulfilled
        m_readyToRenderCv.wait(lock, [this]() {
            return !m_running || (m_window != nullptr && !m_wantPause);
        });

        if (!m_running)
            break;

        m_pause = false;
        lock.unlock();

        deltaClock.ReadDelta();
        stateMachine.update();
        stateMachine.render();
    }
}