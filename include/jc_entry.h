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

extern Uint32 JC_TIMER_EVENT;
int JCEntryInit();

struct JCEventTimerCallbackData {
    cmd_type callback;
    void* userdata;
    
    JCEventTimerCallbackData(cmd_type cb, void* ud) 
        : callback(std::move(cb)), userdata(ud) {}
};
template<int buffer_size = DEFAULT_BUFFER_SIZE>
struct JCEventTimerPacker : JCEventTimer<buffer_size> {
    int createEvent(int timeout, int interval, cmd_type callback, void *userdata = nullptr) {
        JCEntryInit();
        auto data = std::make_shared<JCEventTimerCallbackData>(std::move(callback), userdata);
    
        return this->registerEvent(timeout, interval, 
            [data](void *ptr) {  // shared_ptr 自动管理生命周期
                jclog << "Pushing My event: " << JC_TIMER_EVENT << "\n";
                SDL_Event ev;
                ev.type = JC_TIMER_EVENT;
                ev.user.data1 = data.get();
                SDL_PushEvent(&ev);
                return JC_SUCCESS;
            });
    }
};

struct JCEntry {
    int _running;

    JCTrie<void *> props;
    JCEventCenter ev;
    JCEventTimerPacker<DEFAULT_BUFFER_SIZE> timer;

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
};

#endif // _JCENGINE_ENTRY_H_