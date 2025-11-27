#ifndef _JCENGINE_IMAGE_H_
#define _JCENGINE_IMAGE_H_

#include <string>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <jc_base.h>
#include <jc_entry.h>

struct JCImage {
    SDL_Renderer *ren;
    SDL_Surface *sur;
    SDL_Texture *text;
    SDL_FRect location;

    JCImage(JCEntry& entry);
    int open(const std::string& name);
    bool update();
    void setLoc(SDL_FRect rect);
    void getSize(int *w, int *h);
};

#endif // _JCENGINE_IMAGE_H_