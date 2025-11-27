#ifndef _JCENGINE_ENTRY_CPP_
#define _JCENGINE_ENTRY_CPP_

#include <jc_entry.h>
#include <jc_base.h>
#include <SDL3/SDL.h>

JCEntry::JCEntry(const std::string& name, int width, int height, SDL_WindowFlags winflags) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS)) {
        jclog << "SDL INIT FAILED: " << SDL_GetError() << "\n";
        std::terminate();
    }

    window = SDL_CreateWindow(name.c_str(), width, height, winflags);
    if (window == nullptr) {
        jclog << "Windows Creating Failed: " << SDL_GetError() << '\n';
        std::terminate();
    }

    render = SDL_CreateRenderer(window, nullptr);
    if (render == nullptr) {
        jclog << "Renderer Creating Failed: " << SDL_GetError() << "\n";
        std::terminate();
    }

    ev.registerEvent("quit", [this](void *ptr) {
        this->quit();
        return JC_SUCCESS;
    });
}

int JCEntry::initGPU(SDL_GPUShaderFormat format) {
    gpudev = SDL_CreateGPUDevice(format, 
    #ifdef DEBUG
    true
    #else
    false
    #endif
    , nullptr
    );
    if (gpudev == nullptr) {
        jclog << "GPU Device creating failed: " << SDL_GetError() << "\n";
        return JC_ERROR;
    }
    return JC_SUCCESS;
}

void JCEntry::start(int fps) {
    timer.setTickMS(1);
    _running = 1;
    timer.registerEvent(0, 1000 / fps, [this](void *ptr) {
        this->ev.emitEvent("refresh", this);
        SDL_RenderPresent(this->render);
        return JC_SUCCESS;
    }, this);
}

void JCEntry::mainloop() {
    SDL_Event ev;
    while (_running) {
        while (SDL_PollEvent(&ev)) {
            jclog << "Poll one event\n";
            if (ev.type == SDL_EVENT_QUIT)
                this->ev.emitEvent("quit", this);
        }
        SDL_Delay(1);
    }
}

void JCEntry::quit() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        _running = false;
    }  // 尽早释放锁
    cv.notify_one();
}

void JCEntry::join() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this]{ return !_running; });  // 原子释放锁并休眠
}

#endif // _JCENGINE_ENTRY_CPP_