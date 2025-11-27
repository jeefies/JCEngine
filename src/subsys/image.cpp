#ifndef _JCENGINE_IMAGE_CPP_
#define _JCENGINE_IMAGE_CPP_

#include <jc_image.h>

JCImage::JCImage(JCEntry &entry) {
    ren = entry.render;
}

int JCImage::open(const std::string& name) {
    if (!JCFileExists(name)) return JC_ERROR;
    sur = IMG_Load(name.c_str());
    if (sur == nullptr) return JC_ERROR;
    text = SDL_CreateTextureFromSurface(ren, sur);
    if (text == nullptr) return JC_ERROR;
    return JC_SUCCESS;
}

void JCImage::setLoc(SDL_FRect rect) {
    location = rect;
}

bool JCImage::update() {
    return SDL_RenderTexture(ren, text, NULL, &location);
}

void JCImage::getSize(int *w, int *h) {
    *w = sur->w, *h = sur->h;
}

#endif // _JCENGINE_IMAGE_CPP_