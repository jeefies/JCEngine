#ifndef _JCENGINE_ENTRY_H_
#define _JCENGINE_ENTRY_H_

#include <jc_event.h>
#include <jc_base.h>
#include <SDL3/SDL.h>
#include <atomic>
#include <mutex>
#include <condition_variable>


#define DEFAULT_BUFFER_SIZE 4096
#define DEFAULT_TIMER_TICK 5

struct JCEntry {
    int _running;
    std::mutex mtx;
    std::condition_variable cv;

    JCTrie<void *> props;
    JCEventCenter ev;
    JCEventTimer<DEFAULT_BUFFER_SIZE> timer;

    SDL_Window *window;
    SDL_Renderer *render;
    SDL_GPUDevice *gpudev;

    _DELETE_COPY_MOVE_(JCEntry)

    JCEntry(const std::string& name = "", int width = 1080, int height = 720,
        SDL_WindowFlags winflags = SDL_WINDOW_RESIZABLE);
    int initGPU(SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_SPIRV);
    void start(int fps);
    void quit();
    void mainloop();
    void join();
};

#endif // _JCENGINE_ENTRY_H_