#include <iostream>

#include <SDL3/SDL.h>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

#include <jcengine.h>

JCEntry app("YES!!!");

int main(void) {
    JCImage image(app);
    if (image.open("icon.png")) {
        jclog << "Open icon.png Error: " << SDL_GetError() << "\n";
        return 0;
    }
    
    int imageSize[2]; image.getSize(imageSize, imageSize + 1);
    image.setLoc({0.0, 0.0, (float)(imageSize[0] / 10.0), (float)(imageSize[1] / 10.0)});

    app.ev.registerEvent("refresh", [](void *ptr) {
            // jclog << "Render Clearing...\n";
            SDL_RenderClear(app.render);
            return JC_CONTINUE;
    });

    app.ev.registerEvent("refresh", [&image](void *ptr) {
        // jclog << "Image Updating...\n";
        image.update();
        return JC_CONTINUE;
    });
    
    app.timer.registerEvent(5000, 0, [](void *ptr) {
        SDL_Event ev;
        ev.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&ev);
        return JC_SUCCESS;
    });

    app.start(5);
    app.mainloop();
}