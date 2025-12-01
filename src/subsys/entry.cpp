#ifndef _JCENGINE_ENTRY_CPP_
#define _JCENGINE_ENTRY_CPP_

#include <jc_entry.h>
#include <jc_base.h>
#include <SDL3/SDL.h>

Uint32 JC_TIMER_EVENT = 0;

int JCEntryInit()  {
    static int inited = 0;
    if (!inited) {
        JC_TIMER_EVENT = SDL_RegisterEvents(1);
        inited = 1;
        jclog << "JC_TIMER_EVENT " << JC_TIMER_EVENT << "\n";
    }
    return JC_SUCCESS;
}

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
    timer.createEvent(0, 1000 / fps, [this](void *ptr) {
        jclog << "Timer Emit Refresh\n";
        this->ev.emitEvent("refresh", this);
        SDL_RenderPresent(this->render);
        return JC_SUCCESS;
    }, this);
}

void JCEntry::mainloop() {

    SDL_Event ev;
    while (_running) {
        while (SDL_PollEvent(&ev)) {
            jclog << "Poll one event: " << ev.type << "\n";
            if (ev.type == SDL_EVENT_QUIT)
                this->ev.emitEvent("quit", this);
            if (ev.type == JC_TIMER_EVENT) {
                jclog << "Calling Timer Event !!!\n";
                JCEventTimerCallbackData * data = (JCEventTimerCallbackData *)ev.user.data1;
                data->callback(data->userdata);
            }
        }
        SDL_Delay(1);
    }
}

void JCEntry::quit() {
    _running = false;
}

#endif // _JCENGINE_ENTRY_CPP_